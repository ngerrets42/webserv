#!/usr/local/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()

print("content-type: text/html\n\n")
print("<!doctype html>\n")
print("<html>")
print("<body>")
print("<h1>Directory listing of /upload/ </h1>")
print("<table>")

upload_dir = os.environ['UPLOAD_DIRECTORY'];
files = os.listdir(upload_dir)
if len(files) == 0:
	print("<tr><td> Directory is empty</td></tr>")
for file in files:
	if (os.path.isdir(upload_dir + "/" + file) == False):
		print("<tr>")
		print("<td>" + file + "</td>")
		print("<td> <div id=\"del_button\"><button type=\"button\" onclick=\"loadXMLDoc(\'" + file + "\')\">Delete file</button></div></td>")
		print("</tr>")

print("</table>")
print("<script>")
print("function loadXMLDoc(filename){")
print("var xhttp = new XMLHttpRequest();")
print("xhttp.onreadystatechange = function(){")
print("console.log(\"the staus is:\" + this.status + \" \" + this.readyState)")
print("		if (this.readyState == 4){")
print("				document.open();")
print("				document.write(this.responseText);")
print("				document.close();")
print("		}")
print("};")
print("xhttp.open(\"DELETE\", \"/files/\" + filename, true)")
print("xhttp.send()}")
print("</script>")
print("</body>")
print("</html>")
