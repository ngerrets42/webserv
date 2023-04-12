#!/Users/ngerrets/.brew/bin/lua

print ("Content-type: Text/html\n")
print ("Hello, world! The server's name is: ")

print(os.getenv("SERVER_NAME").."\n")
