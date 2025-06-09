package headers

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestValidSingleHeader(t *testing.T) {
	headers := NewHeaders()
	data := []byte("Host: localhost:42069\r\n\r\n")
	n, done, err := headers.Parse(data)
	require.NoError(t, err)
	require.NotNil(t, headers)
	assert.Equal(t, "localhost:42069", headers["host"])
	assert.Equal(t, 23, n)
	assert.False(t, done)
}

func TestValidSingleHeaderWithExtraWhitespace(t *testing.T) {
	headers := NewHeaders()
	data := []byte("Host:        localhost:42069        \r\n\r\n")
	n, done, err := headers.Parse(data)
	require.NoError(t, err)
	require.NotNil(t, headers)
	assert.Equal(t, "localhost:42069", headers["host"])
	assert.Equal(t, 38, n)
	assert.False(t, done)
}

func TestValid2HeadersWithExistingHeaders(t *testing.T) {
	headers := NewHeaders()
	headers["user-agent"] = "TestAgent"

	data := []byte("Host: localhost:42069\r\nContent-Type: application/json\r\n\r\n")
	n, done, err := headers.Parse(data)
	require.NoError(t, err)
	assert.Equal(t, "localhost:42069", headers["host"])
	assert.Equal(t, "TestAgent", headers["user-agent"])
	assert.Equal(t, 23, n)
	assert.False(t, done)

	remainingData := data[n:]
	n2, done2, err2 := headers.Parse(remainingData)
	require.NoError(t, err2)
	assert.Equal(t, "application/json", headers["content-type"])
	assert.Equal(t, 32, n2)
	assert.False(t, done2)
}

func TestValidDone(t *testing.T) {
	headers := NewHeaders()
	data := []byte("\r\n")
	n, done, err := headers.Parse(data)
	require.NoError(t, err)
	assert.Equal(t, 2, n)
	assert.True(t, done)
	assert.Len(t, headers, 0)
}

func TestInvalidSpacingHeader(t *testing.T) {
	headers := NewHeaders()
	data := []byte("Host : localhost:42069\r\n\r\n")
	n, done, err := headers.Parse(data)
	require.Error(t, err)
	assert.Contains(t, err.Error(), "invalid header field name")
	assert.Equal(t, 0, n)
	assert.False(t, done)
}

func TestCaptialHeader(t *testing.T) {
	headers := NewHeaders()
	data := []byte("Host: localhost:42069\r\n\r\n")
	n, done, err := headers.Parse(data)
	require.NoError(t, err)
	assert.Equal(t, "localhost:42069", headers["host"])
	assert.Equal(t, 23, n)
	assert.False(t, done)
}

func TestInvalidHeaderNameCharacter(t *testing.T) {
	headers := NewHeaders()
	data := []byte("H©st: localhost:42069\r\n\r\n")
	n, done, err := headers.Parse(data)
	require.Error(t, err)
	assert.Contains(t, err.Error(), "invalid header field name")
	assert.Equal(t, 0, n)
	assert.False(t, done)
}
