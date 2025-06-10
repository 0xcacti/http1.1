package request

import (
	"bytes"
	"errors"
	"fmt"
	"io"
	"main/internal/headers"
	"strconv"
	"strings"
	"unicode"
)

const (
	BUFFER_SIZE = 8
	CRLF        = "\r\n"
)

type requestState int

const (
	requestStateInitialized requestState = iota
	requestStateParsingHeaders
	requestStateParsingBody
	requestStateDone
)

type Request struct {
	state       requestState
	RequestLine RequestLine
	Headers     headers.Headers
	Body        []byte
}

type RequestLine struct {
	HttpVersion   string
	RequestTarget string
	Method        string
}

func RequestFromReader(reader io.Reader) (*Request, error) {
	buf := make([]byte, BUFFER_SIZE, BUFFER_SIZE)
	readToIndex := 0
	req := &Request{state: requestStateInitialized, Headers: headers.NewHeaders()}
	for req.state != requestStateDone {
		if readToIndex >= len(buf) {
			newBuf := make([]byte, len(buf)*2)
			copy(newBuf, buf)
			buf = newBuf
		}

		numBytesRead, err := reader.Read(buf[readToIndex:])
		if err != nil {
			if errors.Is(err, io.EOF) {
				if req.state != requestStateDone {
					return nil, fmt.Errorf("unexpected EOF while reading request: %w", err)
				}
				break
			}
			return nil, err
		}

		readToIndex += numBytesRead
		numBytesParsed, err := req.parse(buf[:readToIndex])
		if err != nil {
			return nil, err
		}
		copy(buf, buf[numBytesParsed:])
		readToIndex -= numBytesParsed
	}
	return req, nil
}

func parseRequestLine(data []byte) (*RequestLine, int, error) {
	idx := bytes.Index(data, []byte("\r\n"))
	if idx == -1 {
		return nil, 0, nil
	}

	requestLineText := string(data[:idx])
	requestLine, err := requestLineFromString(requestLineText)
	if err != nil {
		return nil, 0, fmt.Errorf("failed to parse request line: %w", err)
	}
	return requestLine, idx + len(CRLF), nil
}

func requestLineFromString(requestLineStr string) (*RequestLine, error) {
	parts := strings.Split(requestLineStr, " ")
	requestLineParts := strings.Split(requestLineStr, " ")
	if len(parts) != 3 {
		return nil, fmt.Errorf("Poorly formatted request line: %s", requestLineStr)
	}

	method := requestLineParts[0]
	if method != strings.ToUpper(method) || !isAlphabetic(method) {
		return nil, fmt.Errorf("Method not capitalized")
	}
	requestTarget := requestLineParts[1]
	httpVersion := requestLineParts[2]
	if httpVersion != "HTTP/1.1" {
		return nil, fmt.Errorf("We only support HTTP/1.1")
	}
	numericVersionOnly := strings.TrimPrefix(httpVersion, "HTTP/")

	return &RequestLine{
		HttpVersion:   numericVersionOnly,
		RequestTarget: requestTarget,
		Method:        method,
	}, nil
}

func (r *Request) parse(data []byte) (int, error) {
	totalBytesParsed := 0
	for r.state != requestStateDone {
		n, err := r.parseSingle(data[totalBytesParsed:])
		if err != nil {
			return totalBytesParsed, err
		}
		if n == 0 {
			break
		}
		totalBytesParsed += n
	}
	return totalBytesParsed, nil
}

func (r *Request) parseSingle(data []byte) (int, error) {
	switch r.state {
	case requestStateInitialized:
		requestLine, bytesRead, err := parseRequestLine(data)
		if err != nil {
			return 0, err
		}
		if bytesRead == 0 {
			return 0, nil
		}
		r.RequestLine = *requestLine
		r.state = requestStateParsingHeaders
		return bytesRead, nil
	case requestStateParsingHeaders:
		n, done, err := r.Headers.Parse(data)
		if err != nil {
			return 0, fmt.Errorf("failed to parse headers: %w", err)
		}
		if done {
			fmt.Println("Headers parsed successfully moving into body parsing")
			r.state = requestStateParsingBody
		}
		return n, nil

	case requestStateParsingBody:
		fmt.Println("Parsing body")
		if r.Headers.Get("Content-Length") == "" {
			fmt.Println("No Content-Length header found, skipping body parsing")
			r.state = requestStateDone
			return 0, nil
		}
		fmt.Println("Parsing body")
		fmt.Println("Content-Length:", r.Headers.Get("Content-Length"))
		fmt.Println("Current body length:", len(r.Body))
		fmt.Println("Data: ", string(data))

		r.Body = append(r.Body, data...)
		contentLength, err := strconv.Atoi(r.Headers.Get("Content-Length"))
		if err != nil {
			return 0, fmt.Errorf("invalid Content-Length header: %w", err)
		}
		if len(r.Body) > contentLength {
			return 0, fmt.Errorf("error: body length %d exceeds Content-Length %d", len(r.Body), contentLength)
		} else if len(r.Body) == contentLength {
			r.state = requestStateDone
		}
		return len(data), nil
	case requestStateDone:
		return 0, fmt.Errorf("error: trying to read data in a done state")
	default:
		return 0, fmt.Errorf("unknown state %d", r.state)
	}
}

func isAlphabetic(s string) bool {
	for _, r := range s {
		if !unicode.IsLetter(r) {
			return false
		}
	}

	return true
}
