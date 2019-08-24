#include "aws.h"

// ファイルの存在確認関数
inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

// s3バケットへの画像アップロード関数
bool upload_S3_bucket(std::string file_name, std::string fpath)
{
	std::cout << "**********************************" << std::endl;
	std::cout << "Fname : " + fpath + file_name << std::endl;
	std::cout << "**********************************" << std::endl;

    // ファイルの存在確認
    if (!file_exists(fpath + file_name)) {
        std::cerr << "ERROR: NoSuchFile: The specified file does not exist" << std::endl;
        return false;
    }

    // ここから AWS SDKの処理
    Aws::SDKOptions options;
    Aws::InitAPI(options);

	Aws::Client::ClientConfiguration clientConfig;
	clientConfig.region = AWS_REGION;
	clientConfig.scheme = Aws::Http::Scheme::HTTPS;
#ifdef USE_PROXY
	clientConfig.proxyHost  = PROXY_SERVER_NAME;		/* for KonicaMinolta Proxy */
	clientConfig.proxyPort  = PROXY_SERVER_PORT;		/* for KonicaMinolta Proxy */
#endif

    std::string localfilename = fpath + file_name;

	Aws::S3::S3Client s3_client(clientConfig);
    Aws::S3::Model::PutObjectRequest object_request;
    object_request.SetBucket(S3_BUCKET_NAME);
    object_request.SetKey(file_name.c_str());
    const std::shared_ptr<Aws::IOStream> input_data = 
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag", 
                                      localfilename.c_str(), 
                                      std::ios_base::in | std::ios_base::binary);
									  object_request.SetBody(input_data);

    // Put the object
    auto put_object_outcome = s3_client.PutObject(object_request);
    if (!put_object_outcome.IsSuccess()) {
        auto error = put_object_outcome.GetError();
        std::cout << "ERROR: " << error.GetExceptionName() << ": " 
            << error.GetMessage() << std::endl;
        return false;
    }
	Aws::ShutdownAPI(options);
    return true;
}

struct WriteThis {
  const char *readptr;
  size_t sizeleft;
};

 
static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp)
{
	struct WriteThis *wt = (struct WriteThis *)userp;
	size_t buffer_size = size*nmemb;
	if(wt->sizeleft) {
		/* copy as much as possible from the source to the destination */ 
		size_t copy_this_much = wt->sizeleft;
		if(copy_this_much > buffer_size)
		copy_this_much = buffer_size;
		memcpy(dest, wt->readptr, copy_this_much);
		wt->readptr += copy_this_much;
		wt->sizeleft -= copy_this_much;
		return copy_this_much; /* we copied this many bytes */ 
	}
	return 0; /* no more data left to deliver */ 
} 

struct Buffer {
    char *data;
    int data_size;
};

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct Buffer *buf = (struct Buffer *)userp;
    int block = size * nmemb;
    if (!buf) {
        return block;
    }

    if (!buf->data) {
        buf->data = (char *)malloc(block);
    }
    else {
        buf->data = (char *)realloc(buf->data, buf->data_size + block);
    }

    if (buf->data) {
        memcpy(buf->data + buf->data_size, ptr, block);
        buf->data_size += block;
    }

    return block;
}

// 顔認証用サーバにcURLでリクエスト投げて、顔認証する
bool recognition_face_detect(std::string file_name, picojson::object& obj)
{
	// JSONデータの生成
	picojson::object json_data;
	json_data.insert(std::make_pair("file_name", picojson::value(static_cast<std::string>(file_name))));
	json_data.insert(std::make_pair("bucket_name", picojson::value(static_cast<std::string>(S3_BUCKET_NAME))));
	json_data.insert(std::make_pair("threshold", picojson::value(static_cast<float>(MATCH_THRESH))));

	picojson::object json_body;
	json_body.insert(std::make_pair("body", picojson::value(json_data)));

	// JSONデータをstd::stringに変換
	std::string body = picojson::value(json_body).serialize();

	struct WriteThis wt;
	wt.readptr = body.c_str();
	wt.sizeleft = body.size();

    struct Buffer *buf;

    buf = (struct Buffer *)malloc(sizeof(struct Buffer));
    buf->data = NULL;
    buf->data_size = 0;

  	CURL *curl = curl_easy_init(); 
	curl_easy_setopt(curl, CURLOPT_URL, HTTPS_URL); 
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_READDATA, &wt);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 2L);

	struct curl_slist *chunk = NULL; 
	chunk = curl_slist_append(chunk, "Expect:"); 
	
    // APIキーの設定
	std::string api_key = "x-api-key: ";
	api_key += API_KEY;
	chunk = curl_slist_append(chunk,"Content-Type: application/json");
	chunk = curl_slist_append(chunk,api_key.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk); 

	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)wt.sizeleft);
		curl_easy_perform(curl); 

	std::string json = buf->data;
	free(buf->data);
	free(buf);
	curl_easy_cleanup(curl);

	picojson::value v;
	const std::string err = picojson::parse(v, json);
	if (err.empty() == false) {
		std::cerr << err << std::endl;
	}

	picojson::object tobj =  v.get<picojson::object>();

	// Status Code の取得
	uint32_t code = static_cast<uint32_t>(tobj["statusCode"].get<double>());
	// Statusコードがエラーの場合にはメッセージ出力
	if(code != 200){
		return false;
	}

	std::string body_str = picojson::value(tobj["body"]).serialize();
    // 顔認証のJSON出力に不要な'\'文字が含まれているので削除する
	for(size_t c = body_str.find_first_of("\\"); c != std::string::npos; c = body_str.find_first_of("\\")){
		body_str.erase(c,1);
	}
    // 顔認証のJSON出力の前後に不要な'"'文字が含まれているので削除する
	body_str.pop_back();
	body_str.erase(body_str.begin());

	picojson::value vb;
	const std::string berr = picojson::parse(vb, body_str);
	if (berr.empty() == false) {
		std::cerr << berr << std::endl;
	}
	obj  =  vb.get<picojson::object>();                                         // BODY部分のみ抜き出し
	
	return true;
}
