#!/usr/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()

print("content-type: text/html\n\n")
print("<!doctype html>\n")
print("<html>")
print("<body>")
print("<p align=\"center\">Welcome ! </p>")
print("<table>")

upload_dir = os.environ['UPLOAD_DIRECTORY'];
files = os.listdir(upload_dir)

for file in files:
	print("<tr>")
	print("<td>" + file + "</td>")
	print("<td> <div id=\"del_button\"><button type=\"button\" onclick=\"loadXMLDoc()\">Delete file</button></div></td>")
	print("</tr>")

print("</table>")
print("<script>")
print("function loadXMLDoc(){")
print("var xhttp = new XMLHttpRequest();")
print("xhttp.onreadystatechange = function(){")
print("if (this.readyState == 4 && this.status == 200){")
print("document.getElementById(\"del_button\").innerHTML = this.responseText;")
print("}};")
print("xhttp.open(\"GET\", \"/\", true)")
print("xhttp.send()}")
print("</script>")
print("</body>")
print("</html>")
