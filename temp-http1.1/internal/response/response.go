package response

import (
	"fmt"
	"io"
	"main/internal/headers"
	"strconv"
	"strings"
)

type writerState int

const (
	writerStateStatusLine writerState = iota
	writerStateHeaders
	writerStateBody
	writerStateChunkedBody
	writerStateChunkedBodyDone
)

type StatusCode int

const (
	StatusOK            StatusCode = 200
	StatusBadRequest    StatusCode = 400
	StatusInternalError StatusCode = 500
	StatusNotFound      StatusCode = 404
)

type Writer struct {
	w           io.Writer
	writerState writerState
}

func NewWriter(w io.Writer) *Writer {
	return &Writer{
		w:           w,
		writerState: writerStateStatusLine,
	}
}

func (w *Writer) WriteStatusLine(statusCode StatusCode) error {
	if w.writerState != writerStateStatusLine {
		return fmt.Errorf("already wrote status line")
	}

	defer func() { w.writerState = writerStateHeaders }()

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

	_, err := w.w.Write([]byte("HTTP/1.1 " + strconv.Itoa(int(statusCode)) + " " + reasonPhrase + "\r\n"))
	return err
}

func GetDefaultHeaders(contentLen int) headers.Headers {
	h := headers.NewHeaders()
	h["content-length"] = strconv.Itoa(contentLen)
	h["content-type"] = "text/plain"
	h["Connection"] = "close"
	return h
}

func (w *Writer) WriteHeaders(headers headers.Headers) error {
	if w.writerState != writerStateHeaders {
		return fmt.Errorf("writting headers in wrong state: %d", w.writerState)
	}
	defer func() {
		if strings.EqualFold(headers.Get("Transfer-Encoding"), "chunked") {
			w.writerState = writerStateChunkedBody
		} else {
			w.writerState = writerStateBody
		}
	}()

	for key, value := range headers {
		if _, err := w.w.Write([]byte(key + ": " + value + "\r\n")); err != nil {
			return err
		}
	}
	_, err := w.w.Write([]byte("\r\n"))
	return err
}

func (w *Writer) WriteBody(body []byte) (int, error) {
	if w.writerState != writerStateBody {
		return 0, fmt.Errorf("writting body in wrong state: %d", w.writerState)
	}
	defer func() { w.writerState = writerStateStatusLine }()
	return w.w.Write(body)
}

func (w *Writer) WriteChunkedBody(p []byte) (int, error) {
	if w.writerState != writerStateChunkedBody {
		return 0, fmt.Errorf("writting chunked body in wrong state: %d", w.writerState)
	}
	sizeHex := strconv.FormatInt(int64(len(p)), 16)
	if _, err := w.w.Write([]byte(sizeHex + "\r\n")); err != nil {
		return 0, err
	}

	if _, err := w.w.Write(p); err != nil {
		return 0, err
	}

	if _, err := w.w.Write([]byte("\r\n")); err != nil {
		return 0, err
	}

	return len(p), nil
}

func (w *Writer) WriteChunkedBodyDone() (int, error) {
	defer func() { w.writerState = writerStateStatusLine }()
	return w.w.Write([]byte("0\r\n\r\n"))
}
