package request

import (
	"fmt"
	"io"
	"strings"
	"unicode"
)

type RequestLine struct {
	HttpVersion   string
	RequestTarget string
	Method        string
}

type Request struct {
	RequestLine RequestLine
}

func RequestFromReader(reader io.Reader) (*Request, error) {
	req, err := io.ReadAll(reader)
	if err != nil {
		return nil, fmt.Errorf("failed to read request: %w", err)
	}

	reqLine, err := parseRequestLine(string(req))
	if err != nil {
		return nil, err
	}

	return &Request{RequestLine: reqLine}, nil

}

func parseRequestLine(req string) (RequestLine, error) {
	parts := strings.Split(req, "\r\n")
	if len(parts) == 0 {
		return RequestLine{}, fmt.Errorf("No newlines in request, improper format")
	}

	requestLineStr := parts[0]
	requestLineParts := strings.Split(requestLineStr, " ")
	if len(parts) < 3 {
		return RequestLine{}, fmt.Errorf("Cannot parse request-line")
	}

	method := requestLineParts[0]
	if method != strings.ToUpper(method) || !isAlphabetic(method) {
		return RequestLine{}, fmt.Errorf("Method not capitalized")
	}
	requestTarget := requestLineParts[1]
	httpVersion := requestLineParts[2]
	if httpVersion != "HTTP/1.1" {
		return RequestLine{}, fmt.Errorf("We only support HTTP/1.1")
	}
	numericVersionOnly := strings.TrimPrefix(httpVersion, "HTTP/")

	return RequestLine{
		HttpVersion:   numericVersionOnly,
		RequestTarget: requestTarget,
		Method:        method,
	}, nil

}

func isAlphabetic(s string) bool {
	for _, r := range s {
		if !unicode.IsLetter(r) {
			return false
		}
	}

	return true
}
