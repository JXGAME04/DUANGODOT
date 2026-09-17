# KLogin (old S3Client/Login/Login.cpp + Ui/UiCase/UiConnectInfo.cpp): turns the gateway's
# login / kick results into the messages the old client showed (LOGIN_R_* -> LL_R_* -> CI_MI_*).
# Pure functions so the headless tests can check the table.
extends RefCounted

const Proto := preload("res://proto/jx_pb.gd")


# The message for a Result (login failure or kick).  `text` is the gateway's own detail,
# e.g. the reason an account was frozen.
static func result_text(result: int, text: String = "") -> String:
	var detail := " (%s)" % text if text != "" else ""
	match result:
		Proto.Result.OK:
			return ""
		Proto.Result.UNAUTHORIZED:                         # CI_MI_ACCOUNT_PWD_ERROR
			return "Tài khoản hoặc mật khẩu không đúng."
		Proto.Result.ACCOUNT_IN_USE:                       # CI_MI_ACCOUNT_LOCKED (LOGIN_R_ACCOUNT_EXIST)
			return "Tài khoản đang được sử dụng ở nơi khác."
		Proto.Result.ACCOUNT_FROZEN:                       # CI_MI_ACCOUNT_FREEZE
			return "Tài khoản đã bị khoá." + detail
		Proto.Result.NO_GAME_TIME:                         # CI_MI_NOT_ENOUGH_ACCOUNT_POINT
			return "Tài khoản đã hết thời gian chơi."
		Proto.Result.SERVER_BUSY:                          # CI_MI_CONNECT_SERV_BUSY (LOGIN_R_FAILED)
			return "Đăng nhập sai quá nhiều lần hoặc máy chủ bận, hãy thử lại sau."
		Proto.Result.VERSION_MISMATCH:                     # CI_MI_INVALID_PROTOCOLVERSION
			return "Phiên bản client đã cũ, cần cập nhật."
		Proto.Result.ZONE_UNAVAILABLE:
			return "Máy chủ game chưa sẵn sàng."
		Proto.Result.RATE_LIMITED:
			return "Gửi quá nhiều gói tin, kết nối bị ngắt."
		Proto.Result.TIMEOUT:                              # CI_MI_CONNECT_TIMEOUT
			return "Mất kết nối với máy chủ (quá thời gian chờ)."
		Proto.Result.REPLACED:                             # LOGIN_R_BEDISCONNECTED
			return "Tài khoản vừa được đăng nhập ở nơi khác."
		Proto.Result.SERVER_SHUTDOWN:                      # CI_MI_SVRDOWN
			return "Máy chủ đang bảo trì, vui lòng quay lại sau."
		Proto.Result.BAD_REQUEST:
			return "Gói tin không hợp lệ." + detail
		Proto.Result.WRONG_STATE:
			return "Sai trình tự giao thức." + detail
		Proto.Result.INTERNAL_ERROR:
			return "Lỗi máy chủ." + detail
		_:
			return "Lỗi %d%s" % [result, detail]


# After these kicks the gateway closes the socket: the client returns to the login screen (the
# old client went back to the server list, CI_NS_SEL_SERVER).  A zone outage keeps the session:
# the player is back in the character lobby.
static func session_ends(reason: int) -> bool:
	return reason != Proto.Result.ZONE_UNAVAILABLE
