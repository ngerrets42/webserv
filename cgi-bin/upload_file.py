#!/usr/local/bin/python3
import cgi, sys, os
import cgitb; cgitb.enable()
form = cgi.FieldStorage()

message = None

upload_dir = 'var/www/upload/'

message = form

# Test if the file is loaded for the upload
if 'filename' in form:
    fileitem = form['filename']
    fn = os.path.basename(fileitem.filename)
    open(upload_dir + fn, 'wb').write(fileitem.file.read())
    message = 'The file "' + fn + '" was uploaded successfully'
else:
    message = 'No file was uploaded'

# cgi.print_environ()
# cgi.print_environ_usage()
# cgi.print_form(form)

replyhtml = """
<html>
<body>
<p>%s</p>
</body>
</html>
"""
print(replyhtml % message)