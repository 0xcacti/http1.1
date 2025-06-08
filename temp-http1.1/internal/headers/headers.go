package headers

import (
	"bytes"
	"fmt"
	"unicode"
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
	for _, b := range fieldName {
		if unicode.IsSpace(rune(b)) {
			return 0, false, fmt.Errorf("invalid header field name: %s", fieldName)
		}
	}
	fieldName = headerLine[:colonIdx]

	fieldValue := bytes.TrimSpace(headerLine[colonIdx+1:])
	h[string(fieldName)] = string(fieldValue)

	return idx + len(clrf), false, nil
}
