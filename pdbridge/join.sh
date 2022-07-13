#!/bin/sh
# Translate newlines to spaces.
# This is meant to bypass irc penalty rate limiting by combining several lines
# of Pd messages into one irc message. Don't use this for long files.
# Typical usage (while in #pd with irssi):
#  /EXEC -out ./join.sh sub-canvas.txt
#
cat $1 | tr '\n' ' '
