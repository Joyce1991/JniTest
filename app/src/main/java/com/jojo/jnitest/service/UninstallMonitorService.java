package com.jojo.jnitest.service;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import com.jojo.jnitest.UninstallUtil;

public class UninstallMonitorService extends Service {

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i("uninstall", "service onStartCommand");
        UninstallUtil.regUninstallService(this);
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
