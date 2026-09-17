//go:build !windows

package log

import "os"

// prepareConsole: a terminal outside Windows speaks UTF-8 and ANSI already.
func prepareConsole(f *os.File) bool { return isTerminal(f) }
