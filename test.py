import http.client

conn = http.client.HTTPConnection("localhost", 8080)
# Send first request
conn.request("GET", "/index.html")
response = conn.getresponse()
print("First request response: ", response.status, response.reason)

# Send second request
conn.request("GET", "/script.js")
response = conn.getresponse()
print("Second request response: ", response.status, response.reason)

# Send third request
conn.request("GET", "/gr-small.png")
response = conn.getresponse()
print("Third request response: ", response.status, response.reason)

conn.request("GET", "/hello.js")
response = conn.getresponse()
print("4th request response: ", response.status, response.reason)

conn.request("POST", "/")
response = conn.getresponse()
print("5th request response: ", response.status, response.reason)
conn.close()