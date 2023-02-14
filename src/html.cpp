#include "html.h"
#include <dirent.h>

namespace webserv {

	std::string build_index(std::string const& dir_path, std::string const& dir_name)
	{
		std::string page_buffer;

		page_buffer += "<html>\n<head><title>Index of " + dir_name + "</title></head>\n";
		page_buffer += "<h1>Index of " + dir_name + "</h1><hr><pre><a href=\"../\">../</a>\n";

		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir (dir_path.c_str())) == NULL)
		{
			std::cerr << "Can't open directory: " << dir_path << std::endl;
		}
		else
		{
			while ((ent = readdir(dir)) != NULL)
			{
				page_buffer += "<a href=\"";
				page_buffer += ent->d_name;
				if (ent->d_type == DT_DIR)
					page_buffer += '/';
				page_buffer += "\">";
				page_buffer += ent->d_name;
				page_buffer += "</a>\t";
				page_buffer += '-'; // TODO: file size & date
				page_buffer += '\n';
			}
			closedir(dir);
		}		
		page_buffer += "</pre></hr></body>\n</html>\n";
		return (page_buffer);
	}

} // webserv
