#!/usr/local/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()

success = True

message = "Hello there! I'm the delete CGI!"

replyhtml = """
<html>
<body>
<p>%s</p>
</body>
</html>
"""

if success:
	print('content-length: ' + str( len(replyhtml) + len(message) ))

print("content-type: text/html\n")

print(replyhtml % message)
