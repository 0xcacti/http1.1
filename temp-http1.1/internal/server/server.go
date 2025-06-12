package server

import (
	"fmt"
	"io"
	"log"
	"main/internal/request"
	"main/internal/response"
	"net"
	"sync/atomic"
)

type Server struct {
	listener net.Listener
	closed   atomic.Bool
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

type Handler func(w io.Writer, req *request.Request) *HandlerError

func Serve(port int, h Handler) (*Server, error) {
	listener, err := net.Listen("tcp", fmt.Sprintf(":%d", port))
	if err != nil {
		return nil, err
	}
	s := &Server{
		listener: listener,
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
	response.WriteStatusLine(conn, response.StatusOK)
	response.WriteHeaders(conn, response.GetDefaultHeaders(0))
	return
}
