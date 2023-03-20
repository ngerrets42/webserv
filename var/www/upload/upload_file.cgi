#!/usr/local/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()

success = True
message = None
upload_dir = 'files/'

# Test if the file is loaded for the upload
form = cgi.FieldStorage()
if 'filename' in form:
	fileitem = form['filename']
	if len(fileitem.filename) == 0:
		message = 'No file was uploaded'
	else:
		uploaded_file_path = os.path.join(upload_dir, os.path.basename(fileitem.filename))
		with open(uploaded_file_path, 'wb') as fout:
			while True:
				chunk = fileitem.file.read(100000)
				if not chunk:
					break
				fout.write (chunk)
		message = 'The file "' + fileitem.filename + '" was uploaded successfully'
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
