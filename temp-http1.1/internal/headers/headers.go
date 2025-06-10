package headers

import (
	"bytes"
	"fmt"
	"strings"
)

type Headers map[string]string

const clrf = "\r\n"

func NewHeaders() Headers {
	return make(Headers)
}

func (h Headers) Parse(data []byte) (n int, done bool, err error) {
	idx := bytes.Index(data, []byte(clrf))
	if idx == -1 {
		return 0, false, nil
	}
	if idx == 0 {
		return 2, true, nil
	}
	headerLine := data[:idx]
	colonIdx := bytes.Index(headerLine, []byte(":"))
	if colonIdx == -1 {
		return 0, false, nil
	}

	fieldName := bytes.TrimSpace(headerLine[:colonIdx+1])

	for i, b := range fieldName {
		if i == len(fieldName)-1 && b == ':' {
			continue
		}
		if isInvalid(b) {
			return 0, false, fmt.Errorf("invalid header field name: %s", fieldName)
		}
	}
	fieldName = headerLine[:colonIdx]
	fieldName = bytes.ToLower(fieldName)

	fieldValue := bytes.TrimSpace(headerLine[colonIdx+1:])
	if existing, ok := h[string(fieldName)]; ok {
		h[string(fieldName)] = existing + ", " + string(fieldValue)
	} else {
		h[string(fieldName)] = string(fieldValue)
	}

	return idx + len(clrf), false, nil
}

func (h Headers) Get(key string) string {
	key = strings.ToLower(key)
	value := h[key]
	return value
}

func isInvalid(b byte) bool {
	r := rune(b)

	if (r >= 'A' && r <= 'Z') || (r >= 'a' && r <= 'z') || (r >= '0' && r <= '9') {
		return false
	}

	allowedSpecial := "!#$%&'*+-.^_`|~"
	for _, char := range allowedSpecial {
		if r == char {
			return false
		}
	}

	return true
}
