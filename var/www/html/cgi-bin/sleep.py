#!/usr/local/bin/python3
import sys, os, time

print("content-type: text/html\n\n")

message = "You have succesfully slept for 10 seconds!"

while 1:
	i = 0
replyhtml = """
<html>
<body>
<p>%s</p>
</body>
</html>
"""
print(replyhtml % message)