#include "html.h"
#include <dirent.h>
#include <sys/stat.h>

namespace webserv {

	std::string build_index(std::string const& dir_path, std::string const& dir_name)
	{
		std::string page_buffer;
		page_buffer += "<html>\n<head><title>Index of " + dir_name + "</title></head>\n";
		page_buffer += "<h1>Index of " + dir_name + "</h1><hr>\n";

		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir (dir_path.c_str())) == NULL)
		{
			std::cerr << "Can't open directory: " << dir_path << std::endl;
			return (std::string {});
		}
		page_buffer += "<table><tr><th>Name</th><th>Last Modified</th><th>Size (bytes)</th></tr>";
		while ((ent = readdir(dir)) != NULL)
		{
			if (std::string(ent->d_name) == ".")
				continue ;

			std::string filepath = dir_path;
			if(dir_path.back()!= '/'){
				filepath += '/';
			}
			filepath += ent->d_name;

			//get directory/file information
			struct stat buf; //struct to store file/directory data
			stat(filepath.c_str(), &buf);
			char timeline[80]; //string to store c-style string for the time of last modified
			(void)strftime(timeline, 80, "%D %r", localtime(&buf.st_mtimespec.tv_sec));

			page_buffer += "<tr><td>";
			page_buffer += "<a href=\"";
			page_buffer += ent->d_name;
			if (ent->d_type == DT_DIR)
				page_buffer += "/";
			page_buffer += "\">";
			page_buffer += ent->d_name;
			if (ent->d_type == DT_DIR)
				page_buffer += "/";
			page_buffer += "</a></td><td>";
			page_buffer += timeline;
			if (ent->d_type != DT_DIR)
				page_buffer += "</td><td align=\"right\">" + std::to_string(buf.st_size) + "</td>";
			else
				page_buffer += "</td><td align=\"right\"> - </td>";
			page_buffer += '\n';
		}
		closedir(dir);	
		page_buffer += "</table></hr></body>\n</html>\n";
		return (page_buffer);
	}

} // namespace webserv
