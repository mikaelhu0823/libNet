#pragma once
//========================================================================
//[File Name]:Message.h
//[Description]:a implemetation of net message.
//[Author]:Nico Hu
//[Date]:2020-07-20
//[Other]:
//========================================================================

#include <cstddef>
#include <string>


class Message {
public:
	enum { header_len = 8 };
	enum { max_body_len = 65536 };

	const char* data() const {
		return data_;
	}
	char* data() {
		return data_;
	}
	size_t length() const {
		return header_len + bodyLen_;
	}
	const char* body() const {
		return data_ + header_len;
	}
	char* body() {
		return data_ + header_len;
	}
	size_t bodyLen() const {
		return bodyLen_;
	}
	bool decodeHeader() {
		char header[header_len + 1] = "";
		strncat_s(header, data_, header_len);
		bodyLen_ = atoi(header);
		if (bodyLen_ > max_body_len) {
			bodyLen_ = 0;
			return false;
		}
		return true;
	}
	bool decodeData() {
		return decodeHeader();
	}
	void encodeData(const char* data, uint16_t len) {
		setBodyLen(len);
		encodeHeader();
		memcpy_s(data_ + header_len, sizeof(data_) - header_len, data, len);
	}
	void clear() {
		memset(data_, 0, sizeof(data_));
		bodyLen_ = 0;
	}

protected:
	void setBodyLen(size_t len) {
		bodyLen_ = len;
		if (bodyLen_ > max_body_len)
			bodyLen_ = max_body_len;
	}
	void encodeHeader() {
		char header[header_len + 1] = "";
		sprintf_s(header, "%d", bodyLen_);
		memcpy_s(data_, sizeof(data_), header, header_len);
	}

protected:
	char data_[header_len + max_body_len]{0};
	std::size_t bodyLen_{0};
};