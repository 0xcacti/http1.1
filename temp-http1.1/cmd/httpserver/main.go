package main

import (
	"io"
	"log"
	"main/internal/request"
	"main/internal/response"
	"main/internal/server"
	"os"
	"os/signal"
)

const PORT = 42069

func main() {
	server, err := server.Serve(PORT, h)
	if err != nil {
		log.Fatalf("Error starting server: %v", err)
	}

	defer server.Close()
	log.Println("Server started on port", PORT)

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt)
	<-sigChan
	log.Println("Shutting down server...")

}

func h(w io.Writer, req *request.Request) *server.HandlerError {
	if req.RequestLine.RequestTarget == "/yourproblem" {
		return &server.HandlerError{
			Code:    response.StatusBadRequest,
			Message: "Your problem is not my problem\n",
		}
	}

	if req.RequestLine.RequestTarget == "/myproblem" {
		return &server.HandlerError{
			Code:    response.StatusInternalError,
			Message: "Woopsie, my bad\n",
		}
	}

	w.Write([]byte("All good, frfr\n"))
	return nil
}
