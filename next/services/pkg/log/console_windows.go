//go:build windows

package log

import (
	"os"
	"syscall"
)

// prepareConsole makes a Windows console show what the text console prints: UTF-8 output (the
// sentences are Vietnamese; the default OEM page turns them into boxes) and ANSI colours.
// It reports whether colours can be used.
func prepareConsole(f *os.File) bool {
	kernel32 := syscall.NewLazyDLL("kernel32.dll")
	_, _, _ = kernel32.NewProc("SetConsoleOutputCP").Call(65001)
	var mode uint32
	h := syscall.Handle(f.Fd())
	if syscall.GetConsoleMode(h, &mode) != nil {
		return false // not a console (a file, a pipe)
	}
	const enableVirtualTerminalProcessing = 0x0004
	r, _, _ := kernel32.NewProc("SetConsoleMode").Call(uintptr(h), uintptr(mode|enableVirtualTerminalProcessing))
	return r != 0
}
