/*
 *  Copyright (c) 2015-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <fcntl.h>
#include <folly/portability/GTest.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/utils/TestUtils.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

using namespace proxygen;
using namespace std;

TEST(HTTPMessage, TestParseCookiesSimple) {
  HTTPMessage msg;

  msg.getHeaders().add("Cookie", "id=1256679245; data=0:1234567");
  EXPECT_EQ(msg.getCookie("id"), "1256679245");
  EXPECT_EQ(msg.getCookie("data"), "0:1234567");
  EXPECT_EQ(msg.getCookie("mising"), "");
}

TEST(HTTPMessage, TestParseCookiesSpaces) {
  HTTPMessage msg;

  msg.getHeaders().add("Cookie", " id=1256679245 ;   data=0:1234567  ;");
  EXPECT_EQ(msg.getCookie("id"), "1256679245");
  EXPECT_EQ(msg.getCookie("data"), "0:1234567");
}

TEST(HTTPMessage, TestParseCookiesSingleCookie) {
  HTTPMessage msg;

  msg.getHeaders().add("Cookie", "   user_id=1256679245  ");
  EXPECT_EQ(msg.getCookie("user_id"), "1256679245");
}

TEST(HTTPMessage, TestParseCookiesMultipleCookies) {
  HTTPMessage msg;

  msg.getHeaders().add(
    "Cookie", "id=1256679245; data=0:1234567; same=Always; Name");
  msg.getHeaders().add(
    "Cookie", "id2=1256679245; data2=0:1234567; same=Always; ");
  EXPECT_EQ(msg.getCookie("id"), "1256679245");
  EXPECT_EQ(msg.getCookie("id2"), "1256679245");
  EXPECT_EQ(msg.getCookie("data"), "0:1234567");
  EXPECT_EQ(msg.getCookie("data2"), "0:1234567");
  EXPECT_EQ(msg.getCookie("same"), "Always");
  EXPECT_EQ(msg.getCookie("Name"), "");
}

TEST(HTTPMessage, TestParseQueryParamsSimple) {
  HTTPMessage msg;
  string url = "/test?seq=123456&userid=1256679245&dup=1&dup=2&helloWorld"
               "&second=was+it+clear+%28already%29%3F";

  msg.setURL(url);
  EXPECT_EQ(msg.getQueryParam("seq"), "123456");
  EXPECT_EQ(msg.getQueryParam("userid"), "1256679245");
  EXPECT_EQ(msg.getQueryParam("dup"), "2");
  EXPECT_EQ(msg.getQueryParam("helloWorld"), "");
  EXPECT_EQ(msg.getIntQueryParam("dup", 5), 2);
  EXPECT_EQ(msg.getIntQueryParam("abc", 5), 5);
  EXPECT_EQ(msg.getDecodedQueryParam("second"), "was it clear (already)?");
  EXPECT_EQ(msg.getDecodedQueryParam("seq"), "123456");
  EXPECT_EQ(msg.hasQueryParam("seq"), true);
  EXPECT_EQ(msg.hasQueryParam("seq1"), false);
  EXPECT_EQ(msg.getIntQueryParam("dup"), 2);
  EXPECT_ANY_THROW(msg.getIntQueryParam("abc"));
  EXPECT_ANY_THROW(msg.getIntQueryParam("second"));
}

TEST(HTTPMessage, TestParseQueryParamsComplex) {
  HTTPMessage msg;
  std::vector<std::vector<std::string>> input = {
    {"", "", ""},
    {"key_and_equal_but_no_value", "=", ""},
    {"only_key", "", ""},
    {"key_and_value", "=", "value"},
    {"key_and_value_2", "=", "value2"},
    {"key_and_value_3", "=", "value3"}
  };

  for (int i = 0; i < (1 << input.size()); ++i) {
    std::vector<std::vector<std::string>> sub;
    for (size_t j = 0; j < input.size(); ++j) {
      if ((i & (1 << j))) {
        sub.push_back(input[j]);
      }
    }

    sort(sub.begin(), sub.end());
    do {
      bool first = true;
      std::string url = "/test?";
      for (const auto& val: sub) {
        if (first) {
          first = false;
        } else {
          url += "&";
        }

        url += val[0] + val[1] + val[2];
      }

      msg.setURL(url);
      for (const auto& val: sub) {
        if (val[0].empty()) {
          continue;
        }

        EXPECT_EQ(val[2], msg.getQueryParam(val[0]));
      }

    } while (next_permutation(sub.begin(), sub.end()));
  }
}

TEST(HTTPMessage, TestHeaderPreservation) {
  HTTPMessage msg;
  HTTPHeaders& hdrs = msg.getHeaders();

  hdrs.add("Jojo", "1");
  hdrs.add("Binky", "2");
  hdrs.add("jOJo", "3");
  hdrs.add("bINKy", "4");

  EXPECT_EQ(hdrs.size(), 4);
  EXPECT_EQ(hdrs.getNumberOfValues("jojo"), 2);
  EXPECT_EQ(hdrs.getNumberOfValues("binky"), 2);
}

TEST(HTTPMessage, TestHeaderRemove) {
  HTTPMessage msg;
  HTTPHeaders& hdrs = msg.getHeaders();

  hdrs.add("Jojo", "1");
  hdrs.add("Binky", "2");
  hdrs.add("jOJo", "3");
  hdrs.add("bINKy", "4");
  hdrs.remove("jojo");

  EXPECT_EQ(hdrs.size(), 2);
  EXPECT_EQ(hdrs.getNumberOfValues("binky"), 2);
}

TEST(HTTPMessage, TestAllVersionsHeaderRemove) {
  HTTPMessage msg;
  HTTPHeaders& hdrs = msg.getHeaders();

  hdrs.add("Jojo_1_2", "1");
  hdrs.add("Binky", "2");
  hdrs.add("Jojo_1-2", "3");
  hdrs.add("Jojo-1_2", "4");
  hdrs.add("Jojo-1-2", "4");
  hdrs.removeAllVersions(HTTP_HEADER_NONE, "Jojo_1_2");
  EXPECT_EQ(hdrs.size(), 1);

  hdrs.add("Content-Length", "1");
  hdrs.add("Content_Length", "2");
  hdrs.removeAllVersions(HTTP_HEADER_CONTENT_LENGTH, "Content-Length");
  EXPECT_EQ(hdrs.size(), 1);
}

TEST(HTTPMessage, TestSetHeader) {
  HTTPMessage msg;
  HTTPHeaders& hdrs = msg.getHeaders();

  hdrs.set("Jojo", "1");
  EXPECT_EQ(hdrs.size(), 1);
  EXPECT_EQ(hdrs.getNumberOfValues("Jojo"), 1);

  hdrs.add("jojo", "2");
  hdrs.add("jojo", "3");
  hdrs.add("bar", "4");
  EXPECT_EQ(hdrs.size(), 4);
  EXPECT_EQ(hdrs.getNumberOfValues("Jojo"), 3);

  hdrs.set("joJO", "5");
  EXPECT_EQ(hdrs.size(), 2);
  EXPECT_EQ(hdrs.getNumberOfValues("Jojo"), 1);
}

TEST(HTTPMessage, TestCombine) {
  HTTPMessage msg;
  HTTPHeaders& headers = msg.getHeaders();

  EXPECT_EQ(headers.combine("Combine"), "");

  headers.add("Combine", "first value");
  EXPECT_EQ(headers.combine("Combine"), "first value");

  headers.add("Combine", "second value");
  EXPECT_EQ(headers.combine("Combine"), "first value, second value");

  headers.add("Combine", "third value");
  EXPECT_EQ(headers.combine("Combine"),
            "first value, second value, third value");
  VLOG(4) << msg;
}

TEST(HTTPMessage, TestProxification) {
  HTTPMessage msg;

  folly::SocketAddress dstAddr("192.168.1.1", 1887);
  msg.setDstAddress(dstAddr);
  msg.setLocalIp("10.0.0.1");

  msg.ensureHostHeader();
  msg.setWantsKeepalive(false);

  HTTPHeaders& hdrs = msg.getHeaders();
  EXPECT_EQ("192.168.1.1", hdrs.getSingleOrEmpty(HTTP_HEADER_HOST));
  EXPECT_FALSE(msg.wantsKeepalive());
}

TEST(HTTPMessage, TestSetClientAddress) {
  HTTPMessage msg;

  folly::SocketAddress clientAddr("74.125.127.9", 1987);
  msg.setClientAddress(clientAddr);
  EXPECT_EQ(msg.getClientIP(), "74.125.127.9");
  EXPECT_EQ(msg.getClientPort(), "1987");

  // Test updating client address
  folly::SocketAddress clientAddr2("74.125.127.8", 1988);
  msg.setClientAddress(clientAddr2);
  EXPECT_EQ(msg.getClientIP(), "74.125.127.8");
  EXPECT_EQ(msg.getClientPort(), "1988");
}

TEST(HTTPMessage, TestKeepaliveCheck) {
  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 0);
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    EXPECT_TRUE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "close");
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "ClOsE");
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "foo,bar");
    EXPECT_TRUE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "foo,bar");
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "abc,CLOSE,def");
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "xyz");
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "foo,bar");
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "abc ,  CLOSE , def");
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "xyz");
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "  close ");
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, ",  close ");
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 1);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "  close , ");
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 0);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "Keep-Alive");
    EXPECT_TRUE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 0);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "keep-alive");
    EXPECT_TRUE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 0);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "keep-alive");
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "close");
    EXPECT_FALSE(msg.computeKeepalive());
  }

  {
    HTTPMessage msg;
    msg.setHTTPVersion(1, 0);
    msg.getHeaders().add(HTTP_HEADER_CONNECTION, "keep-alive");
    msg.stripPerHopHeaders();
    EXPECT_TRUE(msg.computeKeepalive());
  }
}

TEST(HTTPMessage, TestHeaderStripPerHop) {
  HTTPMessage msg;

  msg.getHeaders().add(HTTP_HEADER_CONNECTION, "a, b, c");
  msg.getHeaders().add(HTTP_HEADER_CONNECTION, "d");
  msg.getHeaders().add(HTTP_HEADER_CONNECTION, ",,,,");
  msg.getHeaders().add(HTTP_HEADER_CONNECTION, " , , , ,");
  msg.getHeaders().add(HTTP_HEADER_CONNECTION, ", e");
  msg.getHeaders().add(HTTP_HEADER_CONNECTION, " f ,\tg\t, \r\n\th ");
  msg.getHeaders().add("Keep-Alive", "true");

  msg.getHeaders().add("a", "1");
  msg.getHeaders().add("b", "2");
  msg.getHeaders().add("c", "3");
  msg.getHeaders().add("d", "4");
  msg.getHeaders().add("e", "5");
  msg.getHeaders().add("f", "6");
  msg.getHeaders().add("g", "7");
  msg.getHeaders().add("h", "8");

  EXPECT_EQ(msg.getHeaders().size(), 15);
  msg.stripPerHopHeaders();
  EXPECT_EQ(msg.getHeaders().size(), 0);
  EXPECT_EQ(msg.getStrippedPerHopHeaders().size(), 15);
}

TEST(HTTPMessage, TestEmptyName) {
  HTTPMessage msg;
  EXPECT_DEATH_NO_CORE(msg.getHeaders().set("", "empty name?!"), ".*");
}

TEST(HTTPMessage, TestMethod) {
  HTTPMessage msg;

  msg.setMethod(HTTPMethod::GET);
  EXPECT_EQ("GET", msg.getMethodString());
  EXPECT_EQ(HTTPMethod::GET == msg.getMethod(), true);

  msg.setMethod("FOO");
  EXPECT_EQ("FOO", msg.getMethodString());
  EXPECT_EQ(folly::none, msg.getMethod());

  msg.setMethod(HTTPMethod::CONNECT);
  EXPECT_EQ("CONNECT", msg.getMethodString());
  EXPECT_EQ(HTTPMethod::CONNECT == msg.getMethod(), true);
}

void testPathAndQuery(const string& url,
                      const string& expectedPath,
                      const string& expectedQuery) {
  HTTPMessage msg;
  msg.setURL(url);

  EXPECT_EQ(msg.getURL(), url);
  EXPECT_EQ(msg.getPath(), expectedPath);
  EXPECT_EQ(msg.getQueryString(), expectedQuery);
}

TEST(GetPathAndQuery, ParseURL) {
  testPathAndQuery("http://localhost:80/foo?bar#qqq", "/foo", "bar");
  testPathAndQuery("localhost:80/foo?bar#qqq", "/foo", "bar");
  testPathAndQuery("localhost", "", "");
  testPathAndQuery("/f/o/o?bar#qqq", "/f/o/o", "bar");
  testPathAndQuery("#?hello", "", "");
}

TEST(HTTPHeaders, AddStringPiece) {
  const char foo[] = "name:value";
  HTTPHeaders headers;

  folly::StringPiece str(foo);
  folly::StringPiece name = str.split_step(':');
  headers.add(name, str);
  EXPECT_EQ("value", headers.getSingleOrEmpty("name"));
}

TEST(HTTPHeaders, InitializerList) {
  HTTPHeaders hdrs;

  hdrs.add({{"name", "value"}});
  hdrs.add({{HTTP_HEADER_CONNECTION, "close"}});
  hdrs.add({{"a", "b"},
            {HTTP_HEADER_CONNECTION, "foo"},
            {HTTP_HEADER_SERVER, "x"}});

  EXPECT_EQ("value", hdrs.getSingleOrEmpty("name"));
  EXPECT_EQ("close, foo", hdrs.combine(HTTP_HEADER_CONNECTION));
  EXPECT_EQ("x", hdrs.getSingleOrEmpty(HTTP_HEADER_SERVER));
}

TEST(HTTPHeaders, InitializerListStringPiece) {
  HTTPHeaders hdrs;

  const char* foo = "name:value";
  folly::StringPiece str(foo);
  folly::StringPiece name = str.split_step(':');
  hdrs.add({{name, str}, {HTTP_HEADER_CONNECTION, str}});

  EXPECT_EQ("value", hdrs.getSingleOrEmpty("name"));
  EXPECT_EQ("value", hdrs.getSingleOrEmpty(HTTP_HEADER_CONNECTION));
}

void testRemoveQueryParam(const string& url,
                          const string& queryParam,
                          const string& expectedUrl,
                          const string& expectedQuery) {
  HTTPMessage msg;
  msg.setURL(url);
  bool expectedChange = (url != expectedUrl);
  EXPECT_EQ(msg.removeQueryParam(queryParam), expectedChange);

  EXPECT_EQ(msg.getURL(), expectedUrl);
  EXPECT_EQ(msg.getQueryString(), expectedQuery);
}

TEST(HTTPMessage, RemoveQueryParamTests) {
  // Query param present
  testRemoveQueryParam("http://localhost:80/foo?param1=a&param2=b#qqq",
                       "param2",
                       "http://localhost:80/foo?param1=a#qqq",
                       "param1=a");
  // Query param not present
  testRemoveQueryParam("http://localhost/foo?param1=a&param2=b#qqq",
                       "param3",
                       "http://localhost/foo?param1=a&param2=b#qqq",
                       "param1=a&param2=b");
  // No scheme
  testRemoveQueryParam("localhost:80/foo?param1=a&param2=b#qqq",
                       "param2",
                       "localhost:80/foo?param1=a#qqq",
                       "param1=a");
  // Just hostname as URL and empty query param
  testRemoveQueryParam("localhost", "param2", "localhost", "");
  testRemoveQueryParam("localhost", "", "localhost", "");
  // Just path as URL
  testRemoveQueryParam("/f/o/o?bar#qqq", "bar", "/f/o/o#qqq", "");
}

void testSetQueryParam(const string& url,
                       const string& queryParam,
                       const string& paramValue,
                       const string& expectedUrl,
                       const string& expectedQuery) {
  HTTPMessage msg;
  msg.setURL(url);
  bool expectedChange = (url != expectedUrl);
  EXPECT_EQ(msg.setQueryParam(queryParam, paramValue), expectedChange);

  EXPECT_EQ(msg.getURL(), expectedUrl);
  EXPECT_EQ(msg.getQueryString(), expectedQuery);
}

TEST(HTTPMessage, SetQueryParamTests) {
  // Overwrite existing parameter
  testSetQueryParam("http://localhost:80/foo?param1=a&param2=b#qqq",
                    "param2",
                    "true",
                    "http://localhost:80/foo?param1=a&param2=true#qqq",
                    "param1=a&param2=true");
  // Add a query parameter
  testSetQueryParam("http://localhost/foo?param1=a&param2=b#qqq",
                    "param3",
                    "true",
                    "http://localhost/foo?param1=a&param2=b&param3=true#qqq",
                    "param1=a&param2=b&param3=true");
  // Add a query parameter, should be alphabetical order
  testSetQueryParam("localhost:80/foo?param1=a&param3=c#qqq",
                    "param2",
                    "b",
                    "localhost:80/foo?param1=a&param2=b&param3=c#qqq",
                    "param1=a&param2=b&param3=c");
  // Add a query parameter when no query parameters exist
  testSetQueryParam("localhost:80/foo#qqq",
                    "param2",
                    "b",
                    "localhost:80/foo?param2=b#qqq",
                    "param2=b");
}

TEST(HTTPMessage, TestCheckForHeaderToken) {
  HTTPMessage msg;
  HTTPHeaders& headers = msg.getHeaders();

  headers.add(HTTP_HEADER_CONNECTION, "HTTP2-Settings");
  EXPECT_TRUE(msg.checkForHeaderToken(HTTP_HEADER_CONNECTION, "HTTP2-Settings",
                                      false));
  EXPECT_FALSE(msg.checkForHeaderToken(HTTP_HEADER_CONNECTION, "http2-settings",
                                       true));
}

TEST(HttpMessage, TestProtocolStringHTTPVersion) {
  HTTPMessage msg;
  msg.setHTTPVersion(1, 1);

  EXPECT_EQ(msg.getProtocolString(), "1.1");
}

TEST(HttpMessage, TestProtocolStringAdvancedProtocol) {
  HTTPMessage msg;
  std::string advancedProtocol = "h2";
  msg.setAdvancedProtocolString(advancedProtocol);
  EXPECT_EQ(msg.getProtocolString(), advancedProtocol);
}

TEST(HttpMessage, TestExtractTrailers) {
  HTTPMessage msg;
  auto trailers = std::make_unique<HTTPHeaders>();
  HTTPHeaders* rawPointer = trailers.get();
  trailers->add("The-trailer", "something");
  msg.setTrailers(std::move(trailers));
  auto trailers2 = msg.extractTrailers();
  EXPECT_EQ(rawPointer, trailers2.get());
  EXPECT_EQ(nullptr, msg.getTrailers());
}
