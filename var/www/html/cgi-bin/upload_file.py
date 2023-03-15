#!/usr/local/bin/python3
import sys, os
sys.stderr.write("upload_file.py: START\n\n\n\n\n\n\n")

sys.stderr.write("LENGTH: \n\n" + os.environ['CONTENT_LENGTH'] + "\n\n")

import cgi, sys, os
import cgitb; cgitb.enable()
sys.stderr.write("LENGTH: \n\n" + os.environ['CONTENT_LENGTH'] + "\n\n")
# try
# 	form = cgi.FieldStorage()
# except IOError, e:
# 	print("Python: Error!")
# 	os.exit()


print("content-type: text/html\n")


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

sys.stderr.write("upload_file.py: END");