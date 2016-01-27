#include <vector>
#include <cstring>
#include <curl/curl.h>

#include "connection.h"

static size_t putBuffer(char *stream, size_t size, size_t nmemb, std::vector<char> *data)
{
	if(data == NULL) {
		return 0;
	}

	size_t ret = 0;
	if(data->size() <= size * nmemb) {
		std::memcpy(stream, data->data(), data->size());
		ret = data->size();
		data->resize(0);
	} else {
		std::memcpy(stream, data->data(), size * nmemb);
		data->erase(data->begin(), data->begin() + (size * nmemb));
		ret = size * nmemb;
	}

	//fprintf(stderr, "put data: %s\n", stream);

	return ret;
}

static int writer(char *data, size_t size, size_t nmemb,
                  std::string *writerData)
{
	if (writerData == NULL)
		return 0;

	writerData->append(data, size*nmemb);

	return size * nmemb;
}

static bool init(CURL *&conn, const char *url, std::string* buffer, char* errorBuffer)
{
	CURLcode code;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	conn = curl_easy_init();

	if (conn == NULL)
	{
		fprintf(stderr, "Failed to create CURL connection\n");

		exit(EXIT_FAILURE);
	}

	curl_easy_setopt(conn, CURLOPT_SSL_VERIFYPEER, 0L);

	code = curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, errorBuffer);
	if (code != CURLE_OK)
	{
		fprintf(stderr, "Failed to set error buffer [%d]\n", code);

		return false;
	}

	code = curl_easy_setopt(conn, CURLOPT_URL, url);
	if (code != CURLE_OK)
	{
		fprintf(stderr, "Failed to set URL [%s]\n", errorBuffer);

		return false;
	}

	code = curl_easy_setopt(conn, CURLOPT_FOLLOWLOCATION, 1L);
	if (code != CURLE_OK)
	{
		fprintf(stderr, "Failed to set redirect option [%s]\n", errorBuffer);

		return false;
	}

	code = curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, writer);
	if (code != CURLE_OK)
	{
		fprintf(stderr, "Failed to set writer [%s]\n", errorBuffer);

		return false;
	}

	code = curl_easy_setopt(conn, CURLOPT_WRITEDATA, buffer);
	if (code != CURLE_OK)
	{
		fprintf(stderr, "Failed to set write data [%s]\n", errorBuffer);

		return false;
	}

	return true;
}

bool downloadJson(std::string url, json_object** output) {
	CURL *conn = NULL;
	char errorBuffer[CURL_ERROR_SIZE];
	std::string buffer;

	if (!init(conn, url.c_str(), &buffer, &errorBuffer[0]))
	{
		fprintf(stderr, "Connection initializion failed\n");
		return false;
	}

	// Retrieve content for the URL

	CURLcode code = curl_easy_perform(conn);
	curl_easy_cleanup(conn);

	if (code != CURLE_OK)
	{
		fprintf(stderr, "Failed to get url %d [%s]\n", code, errorBuffer);
		return false;
	}

	*output = json_tokener_parse(buffer.c_str());
	return *output != NULL;
}

bool postJson(std::string url, json_object* input, json_object** output) {
	CURL *conn = NULL;
	char errorBuffer[CURL_ERROR_SIZE];
	std::string buffer;

	if (!init(conn, url.c_str(), &buffer, &errorBuffer[0]))
	{
		fprintf(stderr, "Connection initializion failed\n");
		return false;
	}

	curl_easy_setopt(conn, CURLOPT_POSTFIELDS, json_object_to_json_string(input));

	// Retrieve content for the URL

	CURLcode code = curl_easy_perform(conn);
	curl_easy_cleanup(conn);

	if (code != CURLE_OK)
	{
		fprintf(stderr, "Failed to get url %d [%s]\n", code, errorBuffer);
		return false;
	}

	*output = json_tokener_parse(buffer.c_str());
	return *output != NULL;
}

bool putJson(std::string url, json_object* input, json_object** output) {
	CURL *conn = NULL;
	char errorBuffer[CURL_ERROR_SIZE];
	std::string buffer;

	if (!init(conn, url.c_str(), &buffer, &errorBuffer[0]))
	{
		fprintf(stderr, "Connection initializion failed\n");
		return false;
	}

	const char* jsonString = json_object_to_json_string(input);
	std::vector<char> data;
	data.resize(strlen(jsonString) + 1);
	memcpy(data.data(), jsonString, strlen(jsonString) + 1);

	curl_easy_setopt (conn, CURLOPT_READFUNCTION, &putBuffer);
	curl_easy_setopt (conn, CURLOPT_READDATA, &data);
	curl_easy_setopt (conn, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt (conn, CURLOPT_INFILESIZE_LARGE, (curl_off_t)(data.size()));

	// Retrieve content for the URL
	CURLcode code = curl_easy_perform(conn);
	curl_easy_cleanup(conn);

	if (code != CURLE_OK)
	{
		fprintf(stderr, "Failed to get url %d [%s]\n", code, errorBuffer);
		return false;
	}

	*output = json_tokener_parse(buffer.c_str());
	return *output != NULL;
}