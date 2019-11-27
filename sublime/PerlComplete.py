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

# If autocomplete requests take longer than this, switch to async completion
SYNC_COMPLETE_THRESHHOLD_MS = 20

# Constants for the status bar
STATUS_KEY = "perl_complete"
STATUS_READY = "PerlComplete ✔"
STATUS_LOADING = "PerlComplete ..."
STATUS_ON_LOAD = "PerlComplete"

debug = True


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


def get_completions(file, context_path, line, col, sigil, word_separators):
    res = get_request("autocomplete", {"path": file, "context": context_path, "line": line, "col": col, "sigil": sigil})
    if not res["success"]:
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


def get_request(url, params={}, attempts=0):
    if attempts > 5:
        log_error("Failed to connect to complete server after 5 attempts")
        return
    try:
        log_debug("Running command url={} params={}".format(url, params))
        res = urllib.request.urlopen("{}{}?{}".format(PERL_COMPLETE_SERVER, url, urllib.parse.urlencode(params)))
        return json.loads(res.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        return json.loads(e.read().decode("utf-8"))
    except urllib.error.URLError as e:
        log_error("Failed to connect to CompleteServer - starting and retrying: {}".format(e))
        start_server()
        return get_request(url, params, attempts + 1)


def write_buffer_to_file(view):
    path = tempfile.gettempdir() + "/PerlComplete.pl"
    with open(path, "w+", encoding="utf-8") as f:
        f.write(view.substr(sublime.Region(0, view.size())))

    return path


# To python, our autocomplete request is just an IO operation (network operation)
# So as soon as our thread starts, it will go into blocked state and so GIL will return control to
# sublime text
class AutoCompleterThread(threading.Thread):
    def __init__(self, on_complete, job_id, path, context_path, line, col, sigil, word_separators):
        super(AutoCompleterThread, self).__init__()
        self.on_complete = on_complete
        self.job_id = job_id

        self.path = path
        self.context = context_path
        self.line = line
        self.col = col
        self.sigil = sigil
        self.word_separators = word_separators

    def run(self):
        completions = get_completions(self.path, self.context, self.line, self.col, self.sigil, self.word_separators)
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

        word_separators = view.settings().get("word_separators")
        current_path = view.window().active_view().file_name()
        # Write current (unsaved) file to a file
        file_data_path = write_buffer_to_file(view)
        current_pos = view.rowcol(view.sel()[0].begin())
        current_pos = (current_pos[0] + 1, current_pos[1] + 1)
        sigil = view.substr(view.line(view.sel()[0]))
        if sigil:
            sigil = sigil[-1]
        else:
            log_error("Could not retrieve sigil context for completion")
            return []

        # if the trigger is not a sigil, don't bother getting completion results
        if not (sigil == "$" or sigil == '@' or sigil == '%'):
            return None

        set_status(view, STATUS_LOADING)

        # return (get_completions(current_path, current_pos[0], current_pos[1], sigil, word_separators), sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)
        job_id = random.randint(1, 100000)
        completion_thread = AutoCompleterThread(self.on_completions_done, job_id, file_data_path, current_path,
                                                current_pos[0], current_pos[1], sigil, word_separators)
        log_info("Starting autocomplete thread with job id {}".format(job_id))
        self.latest_completion_job_id = job_id
        completion_thread.start()

        # TODO at least show default completions
        return None

    def on_load(self, view):
        set_status(view, "")

        if view.settings().get("syntax") != "Packages/Perl/Perl.sublime-syntax":
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


configure_settings()
