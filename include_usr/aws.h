#ifndef __AWS_HEAD__
#define __AWS_HEAD__

#include <iostream>
#include <sstream>
#include <string>
#include <curl/curl.h>
#include "picojson.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

// for S3 upload
#define S3_BUCKET_NAME "sas5-unei-test-zybo"
#define AWS_REGION "ap-northeast-1"
#define USE_PROXY
#ifdef USE_PROXY
#define PROXY_SERVER_NAME "g3.konicaminolta.jp"
#define      PROXY_SERVER_PORT 8080
#endif

// for cURL https requests
#define HTTPS_URL "https://2xn69uqa20.execute-api.ap-northeast-1.amazonaws.com/prod/search"
#define API_KEY "a7ZFbZkNGvaE19f7DOoUj8mtjbrdWCNS7ipRVGo9"
#define MATCH_THRESH 80

bool upload_S3_bucket(std::string file_name, std::string fpath = "./output/");
bool recognition_face_detect(std::string file_name, picojson::object& obj);
#endif //__AWS_HEAD__