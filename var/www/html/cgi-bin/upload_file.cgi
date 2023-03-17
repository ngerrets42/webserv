#!/usr/local/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()
form = cgi.FieldStorage()

succes = true
message = None
upload_dir = 'var/www/upload/files/'

# Test if the file is loaded for the upload
if 'filename' in form:
    fileitem = form['filename']
    fn = os.path.basename(fileitem.filename)
    open(upload_dir + fn, 'wb').write(fileitem.file.read())
    message = 'The file "' + fn + '" was uploaded successfully'
else:
	succes = false
    message = 'No file was uploaded'

replyhtml = """
<html>
<body>
<p>%s</p>
</body>
</html>
"""

if succes:
	print("status: 201 Created")
	print("content-length: " replyhtml.length())

print("content-type: text/html\n")
print(replyhtml % message)
