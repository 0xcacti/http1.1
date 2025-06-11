package main

import (
	"log"
	"main/internal/server"
	"os"
	"os/signal"
)

const PORT = 42069

func main() {
	server, err := server.Serve(PORT)
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
