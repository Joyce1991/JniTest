package com.jojo.jnitest;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

import com.jojo.jnitest.service.UninstallMonitorService;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        TextView word = (TextView) findViewById(R.id.word);
        word.setText(new UninstallUtil().getMsgFromJni());
    }

    @Override
    protected void onStart() {
        super.onStart();
        // 启动JAVA层卸载监听独立进程
        Intent intent = new Intent(MainActivity.this,
                UninstallMonitorService.class);
        startService(intent);
//        UninstallUtil.regUninstallService(this);
    }
}
