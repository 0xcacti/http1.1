package server

import (
	"bytes"
	"fmt"
	"io"
	"log"
	"main/internal/request"
	"main/internal/response"
	"net"
	"sync/atomic"
)

type Handler func(w io.Writer, req *request.Request) *HandlerError

type HandlerError struct {
	Code    response.StatusCode
	Message string
}

func (he HandlerError) Write(w io.Writer) {
	response.WriteStatusLine(w, he.Code)
	messageByes := []byte(he.Message)
	headers := response.GetDefaultHeaders(len(messageByes))
	response.WriteHeaders(w, headers)
	w.Write(messageByes)
}

type Server struct {
	listener net.Listener
	closed   atomic.Bool
	handler  Handler
}

func Serve(port int, h Handler) (*Server, error) {
	listener, err := net.Listen("tcp", fmt.Sprintf(":%d", port))
	if err != nil {
		return nil, err
	}
	s := &Server{
		listener: listener,
		handler:  h,
	}
	go s.listen()
	return s, nil
}

func (s *Server) Close() error {
	s.closed.Store(true)
	if s.listener != nil {
		return s.listener.Close()
	}
	return nil
}

func (s *Server) listen() {
	for {
		conn, err := s.listener.Accept()
		if err != nil {
			if s.closed.Load() {
				return
			}
			log.Printf("Error accepting connection: %v", err)
			continue
		}
		go s.handle(conn)
	}
}

func (s *Server) handle(conn net.Conn) {
	defer conn.Close()
	r, err := request.RequestFromReader(conn)
	if err != nil {
		e := &HandlerError{
			Code:    response.StatusBadRequest,
			Message: fmt.Sprintf("Error reading request: %v", err),
		}
		e.Write(conn)
		return
	}

	buf := bytes.NewBuffer([]byte{})
	hErr := s.handler(buf, r)
	if hErr != nil {
		hErr.Write(conn)
		return
	}

	b := buf.Bytes()
	response.WriteStatusLine(conn, response.StatusOK)
	response.WriteHeaders(conn, response.GetDefaultHeaders(len(b)))
	conn.Write(b)
	return
}
