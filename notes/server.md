Need to consider architecture of the program - should it be a command line program or long running server (HTTP, socket, ...).

#### Command line

##### Pros

- Simple to design, no issues with managing processes
- Cross platform simpler - no messing around with sockets or IPC
- Easy to debug

##### Cons

* Performance impact -
  * Can't do in memory caching - all caching must be loaded from disk



#### Server

##### Pros

* Long running process can cache stuff in memory easily.

##### Cons

* Communication overhead if using HTTP, debug difficulty if using sockets (sockets also hard to implement on sublime end).



We'll go with server as cacheing benefits are good. Need to manage exactly one long running application per computer (not per editor!). If we decide to add per-project state, this can be implemented through the API via some session identification. 

Will use HTTP JSON API for ease of debugging. Not best for performance though. File content for temp buffers transferred via temp files. Need to have some management of long running process.



### JSON protocol

Using HTTP but should be transport agnostic - this means no use of HTTP specific features. All error checking should be done via JSON as well - no use of status codes (but they will be used in addition to the JSON).



```json
{
  method: "autocomplete-variable",
  params: {
    line: 12,
    col: 40,
    path: "/tmp/3948d.pl",
    context: "~/Test/hello.pl",
    sigil: "$"
  }
}
```

Similar to JSON-RPC. 

```
    # panel.run_command("append", {"characters": "/Users/bbr/IdeaProjects/PerlParser/perl/largefile.pl:\n"})
    # panel.run_command("append", {"characters": "  999   my $journal = $req->{journalid}?LJ::load_userid($req->{journalid}):$flags->{'u'};\n"})
    # panel.run_command("append", {"characters": "  1000: my $x = 500;\n"})
    # panel.run_command("append", {"characters": "  1001  $journal = LJ::load_userid($req->{journalid}) or return fail($err, 100);\n"})

```

