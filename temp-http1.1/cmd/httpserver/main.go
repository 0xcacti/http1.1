package main

import (
	"crypto/sha256"
	"fmt"
	"io"
	"log"
	"main/internal/request"
	"main/internal/response"
	"main/internal/server"
	"net/http"
	"os"
	"os/signal"
	"strings"
	"syscall"
)

const port = 42069

func main() {

	server, err := server.Serve(port, handler)
	if err != nil {
		log.Fatalf("Error starting server: %v", err)
	}
	defer server.Close()
	log.Println("Server started on port", port)

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	<-sigChan
	log.Println("Server gracefully stopped")
}

// type Handler func(w io.Writer, req *request.Request)
func handler(w *response.Writer, req *request.Request) {
	if req.RequestLine.RequestTarget == "/yourproblem" {
		handler400(w, req)
		return
	}
	if req.RequestLine.RequestTarget == "/myproblem" {
		handler500(w, req)
		return
	}
	if strings.HasPrefix(req.RequestLine.RequestTarget, "/httpbin") {
		target := strings.TrimPrefix(req.RequestLine.RequestTarget, "/httpbin")
		forwardProxy(w, req, target)
		return
	}
	handler200(w, req)
	return
}

func forwardProxy(w *response.Writer, req *request.Request, target string) {
	resp, err := http.Get("https://httpbin.org" + target)
	if err != nil {
		handler500(w, req)
		return
	}
	defer resp.Body.Close()

	if err := w.WriteStatusLine(response.StatusOK); err != nil {
		handler500(w, req)
		return
	}

	headers := response.GetDefaultHeaders(0)
	headers.Delete("Content-Length")
	headers.Override("Transfer-Encoding", "chunked")
	if err := w.WriteHeaders(headers); err != nil {
		handler500(w, req)
		return
	}

	full := []byte{}
	buf := make([]byte, 1024)
	for {
		n, err := resp.Body.Read(buf)
		if err != nil {
			if err == io.EOF {
				break
			}
			handler500(w, req)
			return
		}
		if n > 0 {
			full = append(full, buf[:n]...)
			if _, werr := w.WriteChunkedBody(buf[:n]); werr != nil {
				return
			}
		}
	}

	if _, err := w.WriteChunkedBodyDone(); err != nil {
		handler500(w, req)
		return
	}

	trailers := response.GetDefaultHeaders(0)
	hash := sha256.Sum256(full)
	trailers.Delete("Content-Length")
	trailers.Add("X-Content-Sha256", fmt.Sprintf("%x", hash))
	trailers.Add("X-Content-Length", fmt.Sprintf("%d", len(full)))
	if err := w.WriteTrailers(trailers); err != nil {
		handler500(w, req)
		return
	}
}

func handler400(w *response.Writer, _ *request.Request) {
	w.WriteStatusLine(response.StatusBadRequest)
	body := []byte(`<html>
<head>
<title>400 Bad Request</title>
</head>
<body>
<h1>Bad Request</h1>
<p>Your request honestly kinda sucked.</p>
</body>
</html>
`)
	h := response.GetDefaultHeaders(len(body))
	h.Override("Content-Type", "text/html")
	w.WriteHeaders(h)
	w.WriteBody(body)
	return
}

func handler500(w *response.Writer, _ *request.Request) {
	w.WriteStatusLine(response.StatusInternalError)
	body := []byte(`<html>
<head>
<title>500 Internal Server Error</title>
</head>
<body>
<h1>Internal Server Error</h1>
<p>Okay, you know what? This one is on me.</p>
</body>
</html>
`)
	h := response.GetDefaultHeaders(len(body))
	h.Override("Content-Type", "text/html")
	w.WriteHeaders(h)
	w.WriteBody(body)
}

func handler200(w *response.Writer, _ *request.Request) {
	w.WriteStatusLine(response.StatusOK)
	body := []byte(`<html>
<head>
<title>200 OK</title>
</head>
<body>
<h1>Success!</h1>
<p>Your request was an absolute banger.</p>
</body>
</html>
`)
	h := response.GetDefaultHeaders(len(body))
	h.Override("Content-Type", "text/html")
	w.WriteHeaders(h)
	w.WriteBody(body)
	return
}
