import sublime
import sublime_plugin
import subprocess
import urllib.request
import urllib.error
import os
import time
import json

PERL_COMPLETE_EXE = "/Users/bbr/IdeaProjects/PerlParser/cmake-build-debug/PerlParser"
PERL_COMPLETE_SERVER = "http://localhost:1234/"

debug = True

def log_info(msg):
    print("[PerlComplete:INFO] - {}".format(msg))

def log_error(msg):
    print("[PerlComplete:ERRO] - {}".format(msg))

def log_debug(msg):
    if debug:
        print("[PerlComplete:DEBG] - {}".format(msg))

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


def get_completions(file, line, col, sigil, word_separators):
    res = get_request("autocomplete", {"path": file, "line": line, "col": col, "sigil": sigil})
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


class PerlCompletionsListener(sublime_plugin.EventListener):
    def on_query_completions(self, view, prefix, locations):
        # TODO move this elsewhere
        view.settings().set("word_separators", "./\\()\"'-,.;<>~!@#$%^&*|+=[]{}`~?")
        # Disable on non-perl files
        if view.settings().get("syntax") != "Packages/Perl/Perl.sublime-syntax":
            return

        word_separators = view.settings().get("word_separators")
        current_path = view.window().active_view().file_name()
        current_pos = view.rowcol(view.sel()[0].begin())
        current_pos = (current_pos[0] + 1, current_pos[1] + 1)
        sigil = view.substr(view.line(view.sel()[0]))
        if sigil:
            sigil = sigil[-1]
        else:
            log_error("Could not retrieve sigil context for completion")
            return []

        return (get_completions(current_path, current_pos[0], current_pos[1], sigil, word_separators),
                sublime.INHIBIT_WORD_COMPLETIONS | sublime.INHIBIT_EXPLICIT_COMPLETIONS)

configure_settings()
