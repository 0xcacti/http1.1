package request

import (
	"bytes"
	"fmt"
	"io"
	"strings"
	"unicode"
)

const bufferSize = 8

type RequestLine struct {
	HttpVersion   string
	RequestTarget string
	Method        string
}

type Request struct {
	state       int
	RequestLine RequestLine
}

func (r *Request) parse(data []byte) (int, error) {
	if r.state == 0 {
		requestLine, bytesRead, err := parseRequestLine(string(data))
		if err != nil {
			return 0, fmt.Errorf("failed to parse request line: %w", err)
		}
		if bytesRead == 0 {
			return 0, nil
		}
		r.RequestLine = requestLine
		r.state = 1
		return bytesRead, nil
	} else if r.state == 1 {
		return 0, fmt.Errorf("error: trying to read data in a done state")
	} else {
		return 0, fmt.Errorf("unknown state %d", r.state)
	}
}

func RequestFromReader(reader io.Reader) (*Request, error) {
	buf := make([]byte, bufferSize, bufferSize)
	readToIndex := 0
	r := &Request{state: 0}
	for r.state != 1 {
		if len(buf) == bufferSize {
			newBuf := make([]byte, cap(buf)*2, cap(buf)*2)
			copy(newBuf, buf)
			buf = newBuf
		}
		n, err := reader.Read(buf[readToIndex:])
		if err != nil {
			if err == io.EOF {
				r.state = 1
				break
			}
			return nil, fmt.Errorf("failed to read from reader: %w", err)
		}
		readToIndex += n
		n, err = r.parse(buf[:readToIndex])
		if err != nil {
			return nil, fmt.Errorf("failed to parse request: %w", err)
		}
		newBuf := make([]byte, 0, bufferSize)
		copy(newBuf, buf[n:])
		readToIndex -= n
	}
	return r, nil
}

func parseRequestLine(req string) (RequestLine, int, error) {
	ctrfIdx := bytes.Index(req, []byte("\r\n"))
	parts := strings.Split(req, "\r\n")
	if len(parts) == 0 {
		return RequestLine{}, 0, nil
	}

	requestLineStr := parts[0]
	requestLineParts := strings.Split(requestLineStr, " ")
	if len(parts) < 3 {
		return RequestLine{}, len(parts), fmt.Errorf("Cannot parse request-line")
	}

	method := requestLineParts[0]
	if method != strings.ToUpper(method) || !isAlphabetic(method) {
		return RequestLine{}, len(parts), fmt.Errorf("Method not capitalized")
	}
	requestTarget := requestLineParts[1]
	httpVersion := requestLineParts[2]
	if httpVersion != "HTTP/1.1" {
		return RequestLine{}, len(parts), fmt.Errorf("We only support HTTP/1.1")
	}
	numericVersionOnly := strings.TrimPrefix(httpVersion, "HTTP/")

	return RequestLine{
		HttpVersion:   numericVersionOnly,
		RequestTarget: requestTarget,
		Method:        method,
	}, len(parts), nil

}

func isAlphabetic(s string) bool {
	for _, r := range s {
		if !unicode.IsLetter(r) {
			return false
		}
	}

	return true
}
