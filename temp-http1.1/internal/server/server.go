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

type Server struct {
	listener net.Listener
	closed   atomic.Bool
	handler  Handler
}

type HandlerError struct {
	Code    response.StatusCode
	Message string
}

func writeError(w io.Writer, e *HandlerError) error {
	if err := response.WriteStatusLine(w, e.Code); err != nil {
		return fmt.Errorf("error writing status line: %w", err)
	}

	headers := response.GetDefaultHeaders(len(e.Message))
	if err := response.WriteHeaders(w, headers); err != nil {
		return fmt.Errorf("error writing headers: %w", err)
	}

	if _, err := w.Write([]byte(e.Message)); err != nil {
		return fmt.Errorf("error writing message: %w", err)
	}

	return nil
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
		if writeErr := writeError(conn, e); writeErr != nil {
			log.Printf("Error writing error response: %v", writeErr)
		}
	}
	buf := bytes.NewBuffer(make([]byte, 1024))
	hErr := s.handler(buf, r)
	if hErr != nil {
		if writeErr := writeError(conn, hErr); writeErr != nil {
			log.Printf("Error writing error response: %v", writeErr)
		}
	}

	response.WriteStatusLine(conn, response.StatusOK)
	response.WriteHeaders(conn, response.GetDefaultHeaders(buf.Len()))
	conn.Write(buf.Bytes())
	return
}
