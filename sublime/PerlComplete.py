import sublime
import sublime_plugin
import subprocess
import urllib.request
import urllib.error
import os
import time
import json
import random
import threading
import tempfile

PERL_COMPLETE_EXE = "/Users/bbr/IdeaProjects/PerlParser/cmake-build-debug/PerlParser"
PERL_COMPLETE_SERVER = "http://localhost:1234/"

# Constants for the status bar
STATUS_KEY = "perl_complete"
STATUS_READY = "PerlComplete ✔"
STATUS_LOADING = "PerlComplete ..."
STATUS_ON_LOAD = "PerlComplete"

debug = True

COMPLETE_SUB = "autocomplete-sub"
COMPLETE_VAR = "autocomplete-var"

POST_ATTEMPTS = 5

# Number of lines difference to split groups in find UX
USAGE_GROUP_THRESHHOLD = 5

def log_info(msg):
    print("[PerlComplete:INFO] - {}".format(msg))


def log_error(msg):
    print("[PerlComplete:ERRO] - {}".format(msg))


def log_debug(msg):
    if debug:
        print("[PerlComplete:DEBG] - {}".format(msg))


def set_status(view, status):
    view.set_status(STATUS_KEY, "")
    view.set_status(STATUS_KEY, status)


def configure_settings():
    settings = sublime.load_settings("Preferences.sublime-settings")
    auto_complete_triggers = settings.get("auto_complete_triggers")

    found = False
    for trigger in auto_complete_triggers:
        if trigger["selector"] == "source.perl":
            found = True
            trigger["characters"] = "$%@"

    if not found:
        auto_complete_triggers.append({"selector": "source.perl", "characters": "$@%"})

    settings.set("auto_complete_triggers", auto_complete_triggers)
    sublime.save_settings("Preferences.sublime-settings")

def get_completions(complete_type, params, word_separators):
    res = post_request(complete_type, params)
    if not res["success"]:
        log_error("Completions failed with error - {}:{}".format(res.get("error"), res.get("errorMessage")))
        return []

    # Convert (completion, detail) to (completion + "\t" + detail, "")
    # In sublime the tab deliminates the two parts
    completions = []
    for completion in res["body"]:
        replacement = completion[0]
        if not replacement:
            continue

        if replacement[0] == "$":
            replacement = "\\" + replacement

        completions.append((completion[0] + "\t" + completion[1], replacement))

    log_info(completions)
    return completions


def find_usages(path, context_path, line, col):
    res = post_request("find-usages", {
        "line": line,
        "col": col,
        "path": path,
        "context": context_path
    })

    if not res["success"]:
        log_error("Find usages failed with error - {}:{}".format(res.get("error"), res.get("errorMessage")))
        return []

    return res


def ping():
    try:
        urllib.request.urlopen(PERL_COMPLETE_SERVER + "ping").read()
    except urllib.error.HTTPError:
        return True  # Bad ping behaviour but at least the server is running
    except urllib.error.URLError:
        return False

    return True


def start_server():
    if not ping():
        log_info("Server stopped - starting again")
        # Could not connect to server, needs to be started
        # First stop any PerlParser processes that may be lying around for some reason
        os.system("killall -9 PerlParser")
        os.system("nohup " + PERL_COMPLETE_EXE + " serve &")

        # Wait for serve to come up
        start = time.time()

        while not ping():
            diff = time.time() - start
            # give up after a second
            if diff > 1:
                log_error("Server didn't start within one second")
    else:
        log_info("Server already running")


def post_request(method, params, attempts=0):
    if attempts > POST_ATTEMPTS:
        log_error("Failed to connect to complete server after 5 attempts")
        return
    try:
        log_debug("Running command method={} params={}".format(method, params))
        post_data = {
            "method": method,
            "params": params
        }

        req = urllib.request.Request(PERL_COMPLETE_SERVER)
        req.add_header('Content-Type', 'application/json; charset=utf-8')
        post_json = json.dumps(post_data).encode("utf-8")
        req.add_header('Content-Length', len(post_json))
        res = urllib.request.urlopen(req, post_json)
        return json.loads(res.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        return json.loads(e.read().decode("utf-8"))
    except urllib.error.URLError as e:
        log_error("Failed to connect to CompleteServer - starting and retrying: {}".format(e))
        start_server()
        return post_request(method, params, attempts + 1)


def write_buffer_to_file(view):
    path = tempfile.gettempdir() + "/PerlComplete.pl"
    with open(path, "w+", encoding="utf-8") as f:
        f.write(view.substr(sublime.Region(0, view.size())))

    return path


def current_view_is_perl():
    current_view = sublime.active_window().active_view()
    return current_view.settings().get("syntax") == "Packages/Perl/Perl.sublime-syntax"


def update_menu():
    # If current view is a perl file, then show the Find Usages in the context menu
    file_path_context = os.path.abspath(os.path.join(os.path.dirname(__file__), "Context.sublime-menu"))

    menu = []
    if current_view_is_perl():
        menu = [{"caption": "-"}, {"caption": "Find Usages", "command": "find_usages"}, {"caption": "-"}, ]

    with open(file_path_context, "w+") as f:
        f.write(json.dumps(menu))


def get_source_line(view, line_regions, line):
    if line >= len(line_regions):
        return None

    return view.substr(line_regions[line - 1])


def show_output_panel(name: str, contents: str):
    win = sublime.active_window()
    panel = win.create_output_panel(name)
    panel.set_syntax_file("Packages/Default/Find Results.hidden-tmLanguage")
    panel.settings().set("result_file_regex", "^([^\\s]+\\.\\w+):?(\\d+)?")
    panel.settings().set("result_line_regex", "  (\\d+):")
    win.run_command('show_panel', {"panel": "output." + name})
    panel.run_command("append", {"characters": contents})


def group_usages(usages):
    result_map = {}
    for file in usages:
        prev_line = None
        groups = []

        for pos in usages[file]:
            line = pos[0]
            if prev_line is not None and line - prev_line < USAGE_GROUP_THRESHHOLD:
                groups[-1].append(line)
            else:
                groups.append([line])
            prev_line = line

        result_map[file] = groups
    return result_map


def build_usage_panel_contents(usage_groups):
    contents = ""

    # Load lines for current view
    view = sublime.active_window().active_view()
    region_lines = view.split_by_newlines(sublime.Region(0, view.size()))

    for file in usage_groups:
        contents += "{}:\n".format(file)

        # If file == currently loaded file, use buffer
        #  Otherwise, read file from disk to split lines
        lines = None
        if file is not None and file != view.file_name():
            if not os.exists(file):
                log_errror("Find usages: File {} does not exist".format(file))
                continue

            lines = open(file).read().splitlines()

        for group in usage_groups[file]:
            # Figure out what lines to list
            lines_to_show = []
            if group[0] > 2:
                lines_to_show.append(group[0] - 2)

            if group[0] > 1:
                lines_to_show.append(group[0] - 1)

            # Now include every line in the group
            for i in range(group[0], group[-1] + 1):
                lines_to_show.append(i)

            # Now we can add the group to the file
            contents += "  .. \n"

            for line in lines_to_show:
                source_line = ""
                if lines is not None:
                    source_line = lines[line]
                else:
                    source_line = get_source_line(view, region_lines, line)

                colon = ":  " if line in group else "   "
                contents += "  {}{}{}\n".format(line, colon, source_line)

    return contents

# To python, our autocomplete request is just an IO operation (network operation)
# So as soon as our thread starts, it will go into blocked state and so GIL will return control to
# sublime text
class AutoCompleterThread(threading.Thread):
    def __init__(self, on_complete, job_id, complete_type, complete_params, word_separators):
        super(AutoCompleterThread, self).__init__()
        self.on_complete = on_complete
        self.job_id = job_id

        self.complete_params = complete_params
        self.complete_type = complete_type
        self.word_separators = word_separators

    def run(self):
        completions = get_completions(self.complete_type, self.complete_params, self.word_separators)
        self.on_complete(self.job_id, completions)


class PerlCompletionsListener(sublime_plugin.EventListener):

    def __init__(self):
        self.completions = None
        self.latest_completion_job_id = None

        # If autocomplete for a specific file
        self.use_async = True

    def on_query_completions(self, view, prefix, locations):
        # TODO move this elsewhere
        view.settings().set("word_separators", "./\\()\"'-,.;<>~!@#$%^&*|+=[]{}`~?")
        # Disable on non-perl files
        if view.settings().get("syntax") != "Packages/Perl/Perl.sublime-syntax":
            set_status(view, "")
            return

        # We have a result from the autocomplete thread
        if self.completions:
            set_status(view, STATUS_READY)
            completion_cpy = self.completions.copy()
            self.completions = None
            return (completion_cpy, sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)

        if self.completions == []:
            # Empty list means no completions, don't try to do any more
            self.completions = None
            return None

        word_separators = view.settings().get("word_separators")
        current_path = view.window().active_view().file_name()
        # Write current (unsaved) file to a file
        file_data_path = write_buffer_to_file(view)
        current_pos = view.rowcol(view.sel()[0].begin())
        current_pos = (current_pos[0] + 1, current_pos[1] + 1)
        sigil = view.substr(view.line(view.sel()[0]))

        autocomplete_method = COMPLETE_VAR
        complete_params = {"line": current_pos[0], "col": current_pos[1], "path": file_data_path,
                           "context": current_path}

        if not sigil or not (sigil[-1] == "$" or sigil[-1] == '@' or sigil[-1] == '%'):
            complete_method = COMPLETE_SUB
        else:
            complete_params["sigil"] = sigil[-1]
            complete_method = COMPLETE_VAR

        set_status(view, STATUS_LOADING)
        job_id = random.randint(1, 100000)
        completion_thread = AutoCompleterThread(self.on_completions_done, job_id, complete_method, complete_params,
                                                word_separators)
        log_info("Starting autocomplete thread with job id {}".format(job_id))
        self.latest_completion_job_id = job_id
        completion_thread.start()

        return None

    def on_activated(self, view):
        update_menu()
        set_status(view, "")

        if not current_view_is_perl():
            return
        else:
            set_status(view, STATUS_ON_LOAD)
            log_info("Loaded perl file, checking server")
            start_server()
            if ping():
                set_status(view, STATUS_READY)


    def on_completions_done(self, job_id, completions):
        log_info("Autocomplete job #{} with completions = {}".format(job_id, completions))
        if job_id != self.latest_completion_job_id:
            log_info("Discarding completion result as job is old: job_id = {}, latest_job_id={}".format(job_id,
                                                                                                        self.latest_completion_job_id))
            return

        self.completions = completions
        view = sublime.active_window().active_view()

        # Hide existing autocomplete popup and retrigger on_query_completions
        view.run_command('hide_auto_complete')
        view.run_command('auto_complete', {
            'disable_auto_insert': True,
            'api_completions_only': False,
            'next_competion_if_showing': False})


class FindUsagesCommand(sublime_plugin.TextCommand):
    def run(self, edit):
        view = sublime.active_window().active_view()
        current_pos = view.rowcol(view.sel()[0].begin())
        file_data_path = write_buffer_to_file(view)
        current_path = view.window().active_view().file_name()

        usages = find_usages(file_data_path, current_path, current_pos[0] + 1, current_pos[1] + 1).get("body")
        if not usages:
            return

        usage_groups = group_usages(usages)
        log_debug("Found usages - {}".format(usage_groups))
        show_output_panel("usages", build_usage_panel_contents(usage_groups))

configure_settings()
update_menu()
