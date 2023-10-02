#include "Engone/Logger.h"

// Temporary
//#include "Engone/Win32Includes.h"

#include "Engone/PlatformLayer.h"
#include "Engone/Asserts.h"


#include <time.h>
#include <string.h>

namespace engone {
	namespace log {
		Logger out;
	}
	// This function is copied from Utilitiy.h to make the logger more independent.
	// str should have space of 8 characters.
	static void _GetClock(char* str) {
		time_t t;
		time(&t);
		#ifdef OS_LINUX
		tm* tmp = localtime(&t);
		#else
		tm _tm;
		tm* tmp = &_tm;
		localtime_s(tmp, &t);
		#endif
		str[2] = ':';
		str[5] = ':';

		int temp = tmp->tm_hour / 10;
		str[0] = '0' + temp;
		temp = tmp->tm_hour - 10 * temp;
		str[1] = '0' + temp;

		temp = tmp->tm_min / 10;
		str[3] = '0' + temp;
		temp = tmp->tm_min - 10 * temp;
		str[4] = '0' + temp;

		temp = tmp->tm_sec / 10;
		str[6] = '0' + temp;
		temp = tmp->tm_sec - 10 * temp;
		str[7] = '0' + temp;
	}
	void Logger::cleanup() {
		flush();
		
		for (auto& pair : m_threadInfos) {
			// pair.second.lineBuffer.resize(0);
            Free(pair.second.lineBuffer_data, pair.second.lineBuffer_max);
		}
		m_threadInfos.clear();
		for(auto& pair : m_logFiles){
			FileClose(pair.second);
		}
		m_logFiles.clear();
	}
	char* Logger::ThreadInfo::ensure(u32 bytes) {
		if (lineBuffer_max < lineBuffer_used + bytes + 1) { // +1 for \0
			// TODO: increase by max*2x instead of used+bytes?
            bool yes = false;
            int newSize = lineBuffer_max*2 + 2*bytes + 1;
            if(!lineBuffer_data) {
                lineBuffer_data = (char*)Allocate(newSize);
                if(!lineBuffer_data)
                    return nullptr;
                lineBuffer_used = 0;
                lineBuffer_max = newSize;
            } else {
                char* ptr = (char*)Reallocate(lineBuffer_data, lineBuffer_max, newSize);
                if(!lineBuffer_data)
                    return nullptr;
                lineBuffer_data = ptr;
                lineBuffer_max = newSize;
            }
			// bool yes = lineBuffer.resize(lineBuffer_max*2 + 2*bytes + 1);
			// if (!yes)
			// 	return nullptr;
		}
		return ((char*)lineBuffer_data + lineBuffer_used);
	}
	void Logger::ThreadInfo::use(u32 bytes) {
		lineBuffer_used += bytes;
		*((char*)lineBuffer_data + lineBuffer_used) = 0;
		//printf("%s",lineBuffer.data+lineBuffer.used);
	}
	void Logger::enableReport(bool yes){
		m_enabledReports = yes;
	}
	void Logger::enableConsole(bool yes){
		m_enabledConsole = yes;
	}
	void Logger::setMasterColor(log::Color color){
		m_masterColor = color;
	}
	void Logger::setMasterReport(const std::string path){
		m_masterReportPath = path;
	}
	void Logger::setReport(const std::string path){
		auto& info = getThreadInfo();
		info.logReport = path;
	}
	void Logger::setIndent(int indent){
		m_indent = indent;
		if(onEmptyLine){
			m_nextIndent = indent;
		}
	}
	void Logger::useThreadReports(bool yes){
		m_useThreadReports=yes;
	}
	Logger::ThreadInfo& Logger::getThreadInfo() {
		auto id = Thread::GetThisThreadId();
		auto find = m_threadInfos.find(id);
		if (find == m_threadInfos.end()) {
			m_threadInfos[id] = {};
		}
		return m_threadInfos[Thread::GetThisThreadId()];
	}
	void Logger::flush(){
		auto& info = getThreadInfo();
		if(info.lineBuffer_used==0)
			return;
		*((char*)info.lineBuffer_data+info.lineBuffer_used) = 0;
		
		char* str = (char*)info.lineBuffer_data;
		int len = info.lineBuffer_used;
		// print((char*)info.lineBuffer.data, info.lineBuffer.used);

		auto outFile = engone::GetStandardOut();

		m_printMutex.lock();
		if (m_masterColor == log::NO_COLOR)
			SetConsoleColor(info.color);
		else
			SetConsoleColor(m_masterColor);

		if(m_enabledConsole){
			if(lastPrintedChar=='\n'){
			// 	preIndent=false;
				for(int j=0;j<m_nextIndent;j++) {
					engone::FileWrite(outFile, " ", 1);
					// fwrite(" ",1, 1, stdout);
				}
				m_nextIndent = m_indent;
			}
			int last = 0;
			for(int i=0;i<len;i++){
				if(str[i]=='\n'||i+1==len){
					engone::FileWrite(outFile, str + last, i+1-last);

					// fwrite(str+last,1,i+1-last,stdout);
					last = i+1;
					if(str[i]=='\n' && i+1!=len){
						for(int j=0;j<m_indent;j++) {
							engone::FileWrite(outFile, " ", 1);
							// fwrite(" ",1, 1, stdout);
						}
					}
				}
			}
			lastPrintedChar = str[len-1];
			if(lastPrintedChar=='\n') {
				onEmptyLine=true;
			}
		}
		if(m_enabledReports){
			const char* te = "[Thread 65536] ";
			char extraBuffer[11+15+1]{0};
			bool printExtra=false;
			if(printExtra){
				extraBuffer[0]='[';
				extraBuffer[9]=']';
				extraBuffer[10]=' ';
				_GetClock(extraBuffer+1);
				sprintf(extraBuffer+11,"[Thread %u] ",Thread::GetThisThreadId());
			}
			if(!m_masterReportPath.empty()){
				std::string path = m_rootDirectory+"/"+m_masterReportPath;
				auto find = m_logFiles.find(path);
				APIFile file={};
				if(find==m_logFiles.end()){
					file = FileOpen(path,nullptr,FILE_ALWAYS_CREATE);
					if(file)
						m_logFiles[path] = file;
				}else{
					file = find->second;
				}
				if(file){
					if(printExtra)
						FileWrite(file,extraBuffer,strlen(extraBuffer));
					u64 bytes = FileWrite(file,str,len);
					// TODO: check for failure
				}
			}
			if(!info.logReport.empty()){
				std::string path = m_rootDirectory+"/"+info.logReport;
				auto find = m_logFiles.find(path);
				APIFile file={};
				if(find==m_logFiles.end()){
					// file = FileOpen(path,0,FILE_CAN_CREATE);
					file = FileOpen(path,nullptr,FILE_ALWAYS_CREATE);
					if(file)
						m_logFiles[path] = file;
				}else{
					file = find->second;
				}
				if(file){
					if(printExtra)
						FileWrite(file,extraBuffer,strlen(extraBuffer));
					u64 bytes = FileWrite(file,str,len);
					// TODO: check for failure
				}
			}
			if(m_useThreadReports){
				std::string path = m_rootDirectory+"/thread";
				path += std::to_string((u32)Thread::GetThisThreadId())+".txt";
				auto find = m_logFiles.find(path);
				APIFile file={};
				if(find==m_logFiles.end()){
					// file = FileOpen(path,0,FILE_CAN_CREATE);
					file = FileOpen(path,nullptr,FILE_ALWAYS_CREATE);
					if(file)
						m_logFiles[path] = file;
				}else{
					file = find->second;
				}
				if(file){
					if(printExtra)
						FileWrite(file,extraBuffer,strlen(extraBuffer));
					u64 bytes = FileWrite(file,str,len);
					// TODO: check for failure
				}
			}
		}
		m_printMutex.unlock();

		info.color = log::SILVER; // reset color after new line
		if (m_masterColor == log::NO_COLOR)
			SetConsoleColor(log::SILVER);

		// TODO: write to report

		info.lineBuffer_used = 0; // flush buffer
	}
	u64 Logger::getMemoryUsage(){
		u64 sum=0;
		for(auto& pair : m_threadInfos){
			sum+=pair.second.lineBuffer_max;
		}
		return sum;
	}
	void Logger::flushInternal() {
		m_printMutex.lock();
		for(auto& pair : m_logFiles){
			engone::FileFlushBuffers(pair.second);
		}
		m_printMutex.unlock();
		engone::FileFlushBuffers(engone::GetStandardOut());
	}
	void Logger::print(char* str, int len) {
		// Assert_platform(str);
		if (len == 0) return;
		
		auto& info = getThreadInfo();
		if(len > 0x1000) {
			log::out << log::YELLOW << "Logger: Printing and ensuring a lot of bytes ("<<len<<"). You flush the bytes multiple times!\n";
		}
		// large
		// TODO: If len is really large then we do some extra flushes so
		//  we don't need to ensure megabytes of data.
		char* buf = info.ensure(len);
		if (buf) {
			memcpy(buf, str, len);
			info.use(len);
			onEmptyLine=false;
		} else {
			printf("[Logger] : Failed ensuring %u bytes\n", len); \
		}
		if (str[len - 1] == '\n') {
			flush();
		}
	}
	log::Color Logger::getColor() {
		auto& info = getThreadInfo();
		if (m_masterColor == log::NO_COLOR)
			return info.color;
		else
			return m_masterColor;
	}
	Logger& Logger::operator<<(log::Color value) {
		auto& inf = getThreadInfo();
		if(inf.color!=value){
			if(inf.lineBuffer_used){
				flush();
			}
			// SetConsoleColor(value); // can't do this with multiple threads
			inf.color = value;
		}
		return *this;
	}
	Logger& Logger::operator<<(log::Filter value) {
		getThreadInfo().filter = value;
		return *this;
	}
	Logger& Logger::operator<<(const char* value) {
		u32 len = strlen(value);

		if (len == 0) return *this;

		// auto& info = getThreadInfo();

		print((char*)value, len);
		// char* buf = info.ensure(len);
		// if (buf) {
		// 	memcpy(buf, value, len);
		// 	info.use(len);
		// 	onEmptyLine=false;
		// } else {
		// 	printf("[Logger] : Failed ensuring %u bytes\n", len); \
		// }
		// if (value[len - 1] == '\n') {
		// 	flush();
		// }
		return *this;
	}
	Logger& Logger::operator<<(char* value) {
		return *this << (const char*)value;
	}
	Logger& Logger::operator<<(char value) {
		char tmp[]{ value,0 };
		return *this << (const char*)&tmp;
	}
	Logger& Logger::operator<<(const std::string& value) {
		return *this << value.c_str();
	}

	// Am I crazy, lazy or smart?
#define GEN_LOG_NUM(TYPE,ENSURE,FORMAT)\
	Logger& Logger::operator<<(TYPE value) {\
		auto& info = getThreadInfo();\
		u32 ensureBytes = ENSURE;\
		char* buf = info.ensure(ensureBytes);\
		if (buf) {\
			int used = sprintf(buf,FORMAT,(TYPE)value);\
			info.use(used);\
			onEmptyLine=false;\
		} else {\
			printf("[Logger] : Failed ensuring %u bytes\n", ensureBytes);\
		}\
		return *this;\
	}

	GEN_LOG_NUM(void*, 18, "%p")
	GEN_LOG_NUM(i64, 21, FORMAT_64"d")
	GEN_LOG_NUM(u64, 20, FORMAT_64"u")
	GEN_LOG_NUM(i32, 11, "%d")
	GEN_LOG_NUM(u32, 10, "%u")
	GEN_LOG_NUM(i16, 6, "%hd")
	GEN_LOG_NUM(u16, 5, "%hu")
	GEN_LOG_NUM(u8, 3, "%hu")
	GEN_LOG_NUM(double, 27, "%.2lf")
	GEN_LOG_NUM(float, 20, "%.2f")

#define GEN_LOG_GEN(TYPE,ENSURE,PRINT)\
	Logger& Logger::operator<<(TYPE value) {\
		auto& info = getThreadInfo();\
		u32 ensureBytes = ENSURE;\
		char* buf = info.ensure(ensureBytes);\
		if (buf) {\
			int used = sprintf PRINT;\
			info.use(used);\
			onEmptyLine=false;\
		} else {\
			printf("[Logger] : Failed ensuring %u bytes\n", ensureBytes);\
		}\
		return *this;\
	}
// #ifdef ENGONE_GLM
	GEN_LOG_GEN(const glm::vec3&,60,(buf,"[%f, %f, %f]",value.x,value.y,value.z))
	GEN_LOG_GEN(const glm::vec4&,80,(buf,"[%f, %f, %f, %f]",value.x,value.y,value.z,value.w))
	GEN_LOG_GEN(const glm::quat&,60,(buf,"[%f, %f, %f, %f]", value.x, value.y, value.z, value.w))
	GEN_LOG_GEN(const glm::mat4&,60,(buf,
		"[%f %f %f %f]\n[%f %f %f %f]\n[%f %f %f %f]\n[%f %f %f %f]",
		value[0].x, value[0].y, value[0].z, value[0].w,
		value[1].x, value[1].y, value[1].z, value[1].w,
		value[2].x, value[2].y, value[2].z, value[2].w,
		value[3].x, value[3].y, value[3].z, value[3].w))
// #endif // ENGONE_GLM

#ifdef ENGONE_PHYSICS
	GEN_LOG_GEN(const rp3d::Vector3&, 60, (buf, "[%f, %f, %f]", value.x, value.y, value.z))
	GEN_LOG_GEN(const rp3d::Quaternion&, 60, (buf, "[%f, %f, %f, %f]", value.x, value.y, value.z, value.w))
#endif // ENGONE_PHYSICS
}

