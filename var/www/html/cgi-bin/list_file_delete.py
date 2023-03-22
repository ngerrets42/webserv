#!/usr/local/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()

success = True
message = None
path = os.environ['UPLOAD_DIRECTORY'];

print("content-type: text/html\n\n")
print("<!doctype html>")
print("<html>")
print("<head>")
print("<title> Directory listing </title>")
print("</head>")
print("<body>")

print("<p> <b>Directory listing </b></p>")
for x in os.listdir(path):
    print("<p>"+ x + "</p>")

print("</body>")
print("</html>")



