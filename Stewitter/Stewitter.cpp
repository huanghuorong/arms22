/*
  Stewitter.cpp - Arduino library to Post messages to Twitter with OAuth.
  Copyright (c) arms22 2010. All right reserved.
*/
/*
  Twitter.cpp - Arduino library to Post messages to Twitter.
  Copyright (c) NeoCat 2009. All right reserved.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
*/

#include <EthernetDNS.h>
#include "Stewitter.h"

#define STEWGATE_POST_API			"/sg1/post/"
#define STEWGATE_LAST_MENTION_API	"/sg1/last_mention/"
#define STEWGATE_HOST				"stewgate.appspot.com"
#define STEWGATE_DOMAIN				"appspot.com"

uint8_t Stewitter::server[4] = {0, 0, 0, 0};

Stewitter::Stewitter(const char *token) : client(Stewitter::server, 80)
{
	deviceToken = token;
}

void Stewitter::print_P(const char *str)
{
	char c;
	do {
		c = (char)pgm_read_byte(str++);
		if(c) {
			client.print(c);
		}
	} while(c);
}

void Stewitter::println_P(const char *str)
{
	print_P(str);
	client.println();
}

static bool is_safe_URL_code(char c)
{
	return (('a' <= c && c <= 'z')	||
			('A' <= c && c <= 'Z')	||
			('0' <= c && c <= '9')	||
			(c == '.')				||
			(c == '-')				||
			(c == '_'));
}

static int percentEscapedLength(const char *str)
{
	int ret = 0;
	char c;
	do {
		c = *str++;
		if (c) {
			if (is_safe_URL_code(c) || c == ' ') {
				ret ++;
			} else {
				ret +=3;		// %xx
			}
		}
	} while (c);
	return ret;
}

void Stewitter::printPercentEscaped(const char *str)
{
	char c;
	do {
		c = *str++;
		if (c) {
			if (is_safe_URL_code(c)) {
				client.print(c);
			} else if (c == ' ') {
				client.print('+');
			} else {
				client.print('%');
				client.print((unsigned char)c,HEX);
			}
		}
	} while (c);
}

bool Stewitter::post(const char *msg)
{
	parseStatus = 0;
	statusCode = 0;

	DNSError err = EthernetDNS.resolveHostName(STEWGATE_DOMAIN, Stewitter::server);
	if (err != DNSSuccess) {
		return false;
	}

	if (client.connect()) {
		println_P(PSTR("POST " STEWGATE_POST_API " HTTP/1.0"));
		println_P(PSTR("Host: " STEWGATE_HOST));
		print_P(PSTR("Content-Length: "));
		client.println(3 +		// "_t="
					   percentEscapedLength(deviceToken) +
					   5 +		// "&msg="
					   percentEscapedLength(msg));
		client.println();
		print_P(PSTR("_t="));
		client.print(deviceToken);
		print_P(PSTR("&msg="));
		printPercentEscaped(msg);
		client.println();
	} else {
		return false;
	}
	return true;
}

bool Stewitter::checkStatus(void)
{
	if (!client.connected()) {
		client.flush();
		client.stop();
		return false;
	}
	if (!client.available())
		return true;
	char c = client.read();
	switch(parseStatus) {
	case 0:
		if (c == ' ') parseStatus++; break;  // skip "HTTP/1.1 "
	case 1:
		if (c >= '0' && c <= '9') {
			statusCode *= 10;
			statusCode += c - '0';
		} else {
			parseStatus++;
		}
	}
	return true;
}

int Stewitter::wait(void)
{
	while (checkStatus());
	return statusCode;
}