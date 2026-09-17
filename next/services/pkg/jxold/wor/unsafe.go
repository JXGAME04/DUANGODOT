package wor

import "unsafe"

func unsafePointer(u *uint32) unsafe.Pointer { return unsafe.Pointer(u) }
