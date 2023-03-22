#!/usr/local/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()

form = cgi.FieldStorage()

name = form.getvalue('name')
if name == None :
    name = "anonymous"

print("content-type: text/html\n\n")
print("<!doctype html>\n")
print("<html>")
print("<body>")
print("<p align=\"center\">Welcome ", name, "! </p>")
print("</body>")
print("</html>")

