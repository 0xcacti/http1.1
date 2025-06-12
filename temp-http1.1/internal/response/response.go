package response

import (
	"io"
	"main/internal/headers"
	"strconv"
)

type StatusCode int

const (
	StatusOK            StatusCode = 200
	StatusBadRequest    StatusCode = 400
	StatusInternalError StatusCode = 500
	StatusNotFound      StatusCode = 404
)

func WriteStatusLine(w io.Writer, statusCode StatusCode) error {
	var reasonPhrase string
	switch statusCode {
	case StatusOK:
		reasonPhrase = "OK"
	case StatusBadRequest:
		reasonPhrase = "Bad Request"
	case StatusNotFound:
		reasonPhrase = "Not Found"
	case StatusInternalError:
		reasonPhrase = "Internal Server Error"
	default:
		reasonPhrase = ""
	}

	_, err := w.Write([]byte("HTTP/1.1 " + strconv.Itoa(int(statusCode)) + " " + reasonPhrase + "\r\n"))
	return err
}

func GetDefaultHeaders(contentLen int) headers.Headers {
	h := headers.NewHeaders()
	h["content-length"] = strconv.Itoa(contentLen)
	h["content-type"] = "text/plain"
	h["Connection"] = "close"
	return h
}

func WriteHeaders(w io.Writer, headers headers.Headers) error {
	for key, value := range headers {
		if _, err := w.Write([]byte(key + ": " + value + "\r\n")); err != nil {
			return err
		}
	}
	_, err := w.Write([]byte("\r\n"))
	return err
}
