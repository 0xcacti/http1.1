package headers

type Headers map[string]string

func (h Headers) Parse(data []byte) (n int, done bool, err error) {
}
