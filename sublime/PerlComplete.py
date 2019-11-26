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


def log_info(msg):
    print("[PerlComplete:INFO] - {}".format(msg))


def log_error(msg):
    print("[PerlComplete:ERROR] - {}".format(msg))


def get_completions(file, line, col):
    res = get_request("autocomplete", {"path": file, "line": line, "col": col})
    if not res["success"]:
        return []

    return res["body"]


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
        log_info("Running command url={} params={}".format(url, params))
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
        # Disable on non-perl files
        if view.settings().get("syntax") != "Packages/Perl/Perl.sublime-syntax":
            return

        loc = locations[0]
        current_path = view.window().active_view().file_name()
        current_pos = view.rowcol(view.sel()[0].begin())
        current_pos = (current_pos[0] + 1, current_pos[1] + 1)
        return get_completions(current_path, current_pos[0], current_pos[1])
