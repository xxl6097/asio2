/*
 * COPYRIGHT (C) 2017, zhllxt
 *
 * author   : zhllxt
 * qq       : 37792738
 * email    : 37792738@qq.com
 *
 */

#ifndef __ASIO2_INI_HPP__
#define __ASIO2_INI_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstring>

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iterator>
#include <unordered_map>

#include <asio2/util/def.hpp>
#include <asio2/util/helper.hpp>

namespace asio2
{

	template<bool is_memory_ini = false> class ini {};

	/**
	 * standard ini file operator class 
	 * each time the function is invoked, it will re open the file and read the content.so if you need to 
	 * get a certain value frequently, you should save the geted value into a variable,then use the variable 
	 * directly instead of calling the function to get the value every time.
	 */
	template<>
	class ini<false>
	{
	public:
		/*
		 * default constructor,the ini file path will be like this : 
		 * eg. if the executed application file is "D:/app/demo.exe",then the ini file is "D:/app/demo.ini"
		 */
		ini()
		{
#if   defined(LINUX)
			std::string dir(PATH_MAX, '\0');
			readlink("/proc/self/exe", (char *)dir.data(), PATH_MAX);
#elif defined(WINDOWS)
			std::string dir(MAX_PATH, '\0');
			dir.resize(::GetModuleFileNameA(NULL, (LPSTR)dir.data(), MAX_PATH));
#endif
			std::string filename;
			auto pos = dir.find_last_of(SLASH);
			if (pos != std::string::npos)
			{
				filename = dir.substr(pos + 1);
				dir.resize(pos + 1);
			}

			pos = filename.find_last_of('.');
			if (pos != std::string::npos)
			{
				filename.resize(pos);
			}

			filename += ".ini";

			m_filepath = dir + filename;

			_make_file();
		}
		/*
		 * is_fullpath = true ,indicate the filename parameter is a full path string
		 * is_fullpath = false ,indicate the filename parameter is a file name string
		 */
		ini(const std::string & file_name_or_path, bool is_fullpath = false)
		{
			if (is_fullpath)
				m_filepath = file_name_or_path;
			else
				m_filepath = get_current_directory() + file_name_or_path;

			_make_file();
		}
		virtual ~ini()
		{
		}

		std::string get_filepath()
		{
			return this->m_filepath;
		}
		std::string set_filepath(const std::string & filepath)
		{
			auto old = std::move(this->m_filepath);
			this->m_filepath = filepath;
			return old;
		}

		bool get(const std::string & sec, const std::string & key, std::string & val)
		{
			std::fstream file(m_filepath, std::fstream::binary | std::fstream::in);
			if (file)
			{
				std::string line;
				std::string s, k, v;
				std::streampos posg;

				char ret;
				while ((ret = this->_getline(file, line, s, k, v, posg)) != 'n')
				{
					switch (ret)
					{
					case 'a':break;
					case 's':
						if (s == sec)
						{
							do
							{
								ret = this->_getline(file, line, s, k, v, posg);
								if (ret == 'k' && k == key)
								{
									val = v;
									return true;
								}
							} while (ret == 'k' || ret == 'a' || ret == 'o');

							return false;
						}
						break;
					case 'k':
						if (s == sec)
						{
							if (k == key)
							{
								val = v;
								return true;
							}
						}
						break;
					case 'o':break;
					default:break;
					}
				}
			}
			return false;
		}

		bool set(const std::string & sec, const std::string & key, const std::string & val)
		{
			std::lock_guard<std::mutex> g(m_lock);
			std::fstream file(m_filepath, std::fstream::binary | std::fstream::in | std::fstream::out);
			if (file)
			{
				std::string line;
				std::string s, k, v;
				std::streampos posg = 0;
				char ret;

				auto update_v = [&]() -> bool
				{
					try
					{
						if (val != v)
						{
							file.clear();
							file.seekg(0, std::ios::end);
							auto filesize = file.tellg();

							std::string buffer;
							auto pos = line.find_first_of('=');
							pos++;
							while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
								pos++;
							buffer += line.substr(0, pos);
							buffer += val;
							buffer += LN;

							int pos_diff = int(line.size() + 1 - buffer.size());

							file.clear();
							file.seekg(posg + std::streampos(line.size() + 1));
							char c;
							while (file.get(c))
							{
								buffer += c;
							}

							if (pos_diff > 0) buffer.append(pos_diff, ' ');

							while (!buffer.empty() && (std::streampos(buffer.size()) + posg > filesize) && buffer.back() == ' ')
								buffer.erase(buffer.size() - 1);

							file.clear();
							file.seekp(posg);
							file.write(buffer.data(), buffer.size());

							return true;
						}
					}
					catch (std::exception &) {}
					return false;
				};

				while ((ret = this->_getline(file, line, s, k, v, posg)) != 'n')
				{
					switch (ret)
					{
					case 'a':break;
					case 's':
						if (s == sec)
						{
							do
							{
								ret = this->_getline(file, line, s, k, v, posg);
								if (ret == 'k' && k == key)
								{
									return update_v();
								}
							} while (ret == 'k' || ret == 'a' || ret == 'o');

							std::string buffer;

							if (posg == std::streampos(-1))
							{
								buffer += LN;

								file.clear();
								file.seekg(0, std::ios::end);
								posg = file.tellg();
							}

							buffer += key;
							buffer += '=';
							buffer += val;
							buffer += LN;

							file.clear();
							file.seekg(posg);
							char c;
							while (file.get(c))
							{
								buffer += c;
							}

							file.clear();
							file.seekp(posg);
							file.write(buffer.data(), buffer.size());

							return true;
						}
						break;
					case 'k':
						if (s == sec)
						{
							if (k == key)
							{
								return update_v();
							}
						}
						break;
					case 'o':break;
					default:break;
					}
				}

				std::string content;

				file.clear();
				file.seekg(0, std::ios::beg);
				char c;
				while (file.get(c))
				{
					content += c;
				}

				if (!content.empty() && content.back() == '\n') content.erase(content.size() - 1);
				if (!content.empty() && content.back() == '\r') content.erase(content.size() - 1);

				std::string buffer;

				if (!sec.empty())
				{
					buffer += '[';
					buffer += sec;
					buffer += ']';
					buffer += LN;
				}

				buffer += key;
				buffer += '=';
				buffer += val;
				buffer += LN;

				if (!sec.empty())
				{
					if (content.empty())
						content = std::move(buffer);
					else
						content = content + LN + buffer;
				}
				else
				{
					if (content.empty())
						content = std::move(buffer);
					else
						content = buffer + content;
				}

				file.clear();
				file.seekp(0, std::ios::beg);
				file.write(content.data(), content.size());

				return true;
			}
			return false;
		}

		std::string get_string(const std::string & sec, const std::string & key, const std::string & default_val = std::string())
		{
			std::string val;
			if (!this->get(sec, key, val))
				val = default_val;
			return val;
		}

		int32_t get_int32(const std::string & sec, const std::string & key, int32_t default_val = 0)
		{
			std::string val;
			return (this->get(sec, key, val) ? std::atoi(val.data()) : default_val);
		}

		bool set_int32(const std::string & sec, const std::string & key, int32_t val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		uint32_t get_uint32(const std::string & sec, const std::string & key, uint32_t default_val = 0)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<uint32_t>(std::strtoul(val.data(), nullptr, 10)) : default_val);
		}

		bool set_uint32(const std::string & sec, const std::string & key, uint32_t val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		int64_t get_int64(const std::string & sec, const std::string & key, int64_t default_val = 0)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<int64_t>(std::strtoll(val.data(), nullptr, 10)) : default_val);
		}

		bool set_int64(const std::string & sec, const std::string & key, int64_t val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		uint64_t get_uint64(const std::string & sec, const std::string & key, uint64_t default_val = 0)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<uint64_t>(std::strtoull(val.data(), nullptr, 10)) : default_val);
		}

		bool set_uint64(const std::string & sec, const std::string & key, uint64_t val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		float get_float(const std::string & sec, const std::string & key, float default_val = 0.0f)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<float>(std::strtof(val.data(), nullptr)) : default_val);
		}

		bool set_float(const std::string & sec, const std::string & key, float val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		double get_double(const std::string & sec, const std::string & key, double default_val = 0.0)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<double>(std::strtod(val.data(), nullptr)) : default_val);
		}

		bool set_double(const std::string & sec, const std::string & key, double val)
		{
			return this->set(sec, key, std::to_string(val));
		}

	protected:
		char _getline(std::fstream & file, std::string & line, std::string & sec, std::string & key, std::string & val, std::streampos & posg)
		{
			if (file.is_open())
			{
				posg = file.tellg();

				file.clear();

				if (posg != std::streampos(-1) && std::getline(file, line))
				{
					auto trim_line = line;

					trim_both(trim_line);

					// current line is code annotation
					if (
						(trim_line.size() > 0 && (trim_line[0] == ';' || trim_line[0] == '#' || trim_line[0] == ':'))
						||
						(trim_line.size() > 1 && trim_line[0] == '/' && trim_line[1] == '/')
						)
					{
						return 'a'; // annotation
					}

					auto pos1 = trim_line.find_first_of('[');
					auto pos2 = trim_line.find_first_of(']');

					// current line is section
					if (
						pos1 == 0
						&&
						pos2 != std::string::npos
						&&
						pos2 > pos1
						)
					{

						sec = trim_line.substr(pos1 + 1, pos2 - pos1 - 1);

						trim_both(sec);

						return 's'; // section
					}

					auto sep = trim_line.find_first_of('=');

					// current line is key and val
					if (sep != std::string::npos && sep > 0)
					{
						key = trim_line.substr(0, sep);
						trim_both(key);

						val = trim_line.substr(sep + 1);
						trim_both(val);

						return 'k'; // kv
					}

					return 'o'; // other
				}
			}

			return 'n'; // null
		}

		void _make_file()
		{
			std::lock_guard<std::mutex> g(m_lock);
			std::fstream(m_filepath, std::fstream::binary | std::fstream::app | std::fstream::out);
		}

	protected:
		std::string    m_filepath;

		std::mutex     m_lock;

	};

	/**
	 * standard memory string ini operator class 
	 */
	template<>
	class ini<true>
	{
	public:
		ini()
		{
		}
		ini(const std::string & content) : m_content(content)
		{
		}
		virtual ~ini()
		{
		}

		const std::string & get_content()
		{
			return this->m_content;
		}
		const std::string & set_content(const std::string & content)
		{
			this->m_content = content;
			return this->m_content;
		}

		bool get(const std::string & sec, const std::string & key, std::string & val)
		{
			std::stringstream file(this->m_content);
			if (file)
			{
				std::string line;
				std::string s, k, v;
				std::streampos posg;

				char ret;
				while ((ret = this->_getline(file, line, s, k, v, posg)) != 'n')
				{
					switch (ret)
					{
					case 'a':break;
					case 's':
						if (s == sec)
						{
							do
							{
								ret = this->_getline(file, line, s, k, v, posg);
								if (ret == 'k' && k == key)
								{
									val = v;
									return true;
								}
							} while (ret == 'k' || ret == 'a' || ret == 'o');

							return false;
						}
						break;
					case 'k':
						if (s == sec)
						{
							if (k == key)
							{
								val = v;
								return true;
							}
						}
						break;
					case 'o':break;
					default:break;
					}
				}
			}
			return false;
		}

		bool set(const std::string & sec, const std::string & key, const std::string & val)
		{
			std::lock_guard<std::mutex> g(m_lock);
			std::stringstream file(this->m_content);
			if (file)
			{
				std::string line;
				std::string s, k, v;
				std::streampos posg = 0;
				char ret;

				auto update_v = [&]() -> bool
				{
					try
					{
						if (val != v)
						{
							file.clear();
							file.seekg(0, std::ios::end);
							auto filesize = file.tellg();

							std::string buffer;
							auto pos = line.find_first_of('=');
							pos++;
							while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
								pos++;
							buffer += line.substr(0, pos);
							buffer += val;
							buffer += LN;

							int pos_diff = int(line.size() + 1 - buffer.size());

							file.clear();
							file.seekg(posg + std::streampos(line.size() + 1));
							char c;
							while (file.get(c))
							{
								buffer += c;
							}

							if (pos_diff > 0) buffer.append(pos_diff, ' ');

							while (!buffer.empty() && (std::streampos(buffer.size()) + posg > filesize) && buffer.back() == ' ')
								buffer.erase(buffer.size() - 1);

							file.clear();
							file.seekp(posg);
							file.write(buffer.data(), buffer.size());

							this->m_content = file.str();

							return true;
						}
					}
					catch (std::exception &) {}
					return false;
				};

				while ((ret = this->_getline(file, line, s, k, v, posg)) != 'n')
				{
					switch (ret)
					{
					case 'a':break;
					case 's':
						if (s == sec)
						{
							do
							{
								ret = this->_getline(file, line, s, k, v, posg);
								if (ret == 'k' && k == key)
								{
									return update_v();
								}
							} while (ret == 'k' || ret == 'a' || ret == 'o');

							std::string buffer;

							if (posg == std::streampos(-1))
							{
								buffer += LN;

								file.clear();
								file.seekg(0, std::ios::end);
								posg = file.tellg();
							}

							buffer += key;
							buffer += '=';
							buffer += val;
							buffer += LN;

							file.clear();
							file.seekg(posg);
							char c;
							while (file.get(c))
							{
								buffer += c;
							}

							file.clear();
							file.seekp(posg);
							file.write(buffer.data(), buffer.size());

							this->m_content = file.str();

							return true;
						}
						break;
					case 'k':
						if (s == sec)
						{
							if (k == key)
							{
								return update_v();
							}
						}
						break;
					case 'o':break;
					default:break;
					}
				}

				std::string content;

				file.clear();
				file.seekg(0, std::ios::beg);
				char c;
				while (file.get(c))
				{
					content += c;
				}

				if (!content.empty() && content.back() == '\n') content.erase(content.size() - 1);
				if (!content.empty() && content.back() == '\r') content.erase(content.size() - 1);

				std::string buffer;

				if (!sec.empty())
				{
					buffer += '[';
					buffer += sec;
					buffer += ']';
					buffer += LN;
				}

				buffer += key;
				buffer += '=';
				buffer += val;
				buffer += LN;

				if (!sec.empty())
				{
					if (content.empty())
						content = std::move(buffer);
					else
						content = content + LN + buffer;
				}
				else
				{
					if (content.empty())
						content = std::move(buffer);
					else
						content = buffer + content;
				}

				file.clear();
				file.seekp(0, std::ios::beg);
				file.write(content.data(), content.size());

				this->m_content = file.str();

				return true;
			}
			return false;
		}

		std::string get_string(const std::string & sec, const std::string & key, const std::string & default_val = std::string())
		{
			std::string val;
			if (!this->get(sec, key, val))
				val = default_val;
			return val;
		}

		int32_t get_int32(const std::string & sec, const std::string & key, int32_t default_val = 0)
		{
			std::string val;
			return (this->get(sec, key, val) ? std::atoi(val.data()) : default_val);
		}

		bool set_int32(const std::string & sec, const std::string & key, int32_t val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		uint32_t get_uint32(const std::string & sec, const std::string & key, uint32_t default_val = 0)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<uint32_t>(std::strtoul(val.data(), nullptr, 10)) : default_val);
		}

		bool set_uint32(const std::string & sec, const std::string & key, uint32_t val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		int64_t get_int64(const std::string & sec, const std::string & key, int64_t default_val = 0)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<int64_t>(std::strtoll(val.data(), nullptr, 10)) : default_val);
		}

		bool set_int64(const std::string & sec, const std::string & key, int64_t val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		uint64_t get_uint64(const std::string & sec, const std::string & key, uint64_t default_val = 0)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<uint64_t>(std::strtoull(val.data(), nullptr, 10)) : default_val);
		}

		bool set_uint64(const std::string & sec, const std::string & key, uint64_t val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		float get_float(const std::string & sec, const std::string & key, float default_val = 0.0f)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<float>(std::strtof(val.data(), nullptr)) : default_val);
		}

		bool set_float(const std::string & sec, const std::string & key, float val)
		{
			return this->set(sec, key, std::to_string(val));
		}

		double get_double(const std::string & sec, const std::string & key, double default_val = 0.0)
		{
			std::string val;
			return (this->get(sec, key, val) ? static_cast<double>(std::strtod(val.data(), nullptr)) : default_val);
		}

		bool set_double(const std::string & sec, const std::string & key, double val)
		{
			return this->set(sec, key, std::to_string(val));
		}

	protected:
		char _getline(std::stringstream & file, std::string & line, std::string & sec, std::string & key, std::string & val, std::streampos & posg)
		{
			if (file)
			{
				posg = file.tellg();

				file.clear();

				if (posg != std::streampos(-1) && std::getline(file, line))
				{
					auto trim_line = line;

					trim_both(trim_line);

					// current line is code annotation
					if (
						(trim_line.size() > 0 && (trim_line[0] == ';' || trim_line[0] == '#' || trim_line[0] == ':'))
						||
						(trim_line.size() > 1 && trim_line[0] == '/' && trim_line[1] == '/')
						)
					{
						return 'a'; // annotation
					}

					auto pos1 = trim_line.find_first_of('[');
					auto pos2 = trim_line.find_first_of(']');

					// current line is section
					if (
						pos1 == 0
						&&
						pos2 != std::string::npos
						&&
						pos2 > pos1
						)
					{

						sec = trim_line.substr(pos1 + 1, pos2 - pos1 - 1);

						trim_both(sec);

						return 's'; // section
					}

					auto sep = trim_line.find_first_of('=');

					// current line is key and val
					if (sep != std::string::npos && sep > 0)
					{
						key = trim_line.substr(0, sep);
						trim_both(key);

						val = trim_line.substr(sep + 1);
						trim_both(val);

						return 'k'; // kv
					}

					return 'o'; // other
				}
			}

			return 'n'; // null
		}

	protected:
		std::string    m_content;

		std::mutex     m_lock;

	};
}

#endif // !__ASIO2_INI_HPP__
