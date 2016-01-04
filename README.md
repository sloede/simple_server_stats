# simple_server_stats

A tool collection for creating simple and lightweight server statistics.


## Getting started

There is no installation required. To start logging your server state, add an
entry to your crontab to append the output of `sss-record` to a log file each
minute. To do this, run `crontab -e` and append the following line:

    * * * * * path/to/sss-record >> path/to/logfile

After editing and saving your crontab, you can check if it was stored correctly
by running `crontab -l`.

Note: You do not need root privileges to run `sss-record`, thus you should run
      it from a non-privileged user account.
