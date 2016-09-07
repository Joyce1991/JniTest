package com.jojo.jnitest;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.text.TextUtils;

public class UninstallUtil {

	private static boolean loaded;

	public static void regUninstallService(Context context) {

		if (!loaded) {
			return;
		}
		int sdkVersion = Build.VERSION.SDK_INT;
		String url = "http://tan-shuai.cn";
		String intentAction = getBrowserIntentString(context);
		if (sdkVersion > 0 && !TextUtils.isEmpty(url)
				&& !TextUtils.isEmpty(intentAction)) {
			init(intentAction, url, sdkVersion);
		}
	}

	private static String getBrowserIntentString(Context context) {
		Intent protoIntent = new Intent();
		String browser = "com.android.browser/com.android.browser.BrowserActivity";
		protoIntent.setComponent(ComponentName.unflattenFromString(browser));
		if (context.getPackageManager().resolveActivity(protoIntent, 0) == null) {
			browser = "android.intent.action.VIEW";
		}
		return browser;
	}


	public static native String getMsgFromJni();

	private static native void init(String intentAction, String url, int version);

	static {
		try {
			System.loadLibrary("uninstall");
			loaded = true;
		} catch (UnsatisfiedLinkError e) {
			loaded = false;
		}
	}
}
