#!/usr/local/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()
form = cgi.FieldStorage()

success = True
message = None
upload_dir = 'var/www/upload/files/'

# Test if the file is loaded for the upload
if 'filename' in form:
	fileitem = form['filename']
	fn = os.path.basename(fileitem.filename)
	open(upload_dir + fn, 'wb').write(fileitem.file.read())
	message = 'The file "' + fn + '" was uploaded successfully'
else:
	success = False
	message = 'No file was uploaded'

replyhtml = """
<html>
<body>
<p>%s</p>
</body>
</html>
"""

if success:
	print("content-length: " + str( len(replyhtml) + len(message) ))
	# print("status: 201 Created")

print("content-type: text/html\n")
print(replyhtml % message)
