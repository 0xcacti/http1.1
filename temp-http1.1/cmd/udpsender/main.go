package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"os"
)

func main() {
	network, err := net.ResolveUDPAddr("udp", "localhost:42069")
	if err != nil {
		log.Fatal(err)
	}
	conn, err := net.DialUDP("udp", nil, network)
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()
	reader := bufio.NewReader(os.Stdin)

	for {
		fmt.Print("> ")
		text, _, err := reader.ReadLine()
		if err != nil {
			log.Println(err)
		}
		_, err = conn.Write(text)
		if err != nil {
			log.Println(err)
		}
	}

}
