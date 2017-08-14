package com.example.efrenyang.helloworld;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

/**
 * Created by efrenyang on 2017/6/28.
 */

public class Hellojni extends Activity {
    public void onCreate(Bundle saveInstance)
    {
        super.onCreate(saveInstance);

        TextView tv = new TextView(this);
        tv.setText(stringFromJni());
        //tv.setText(unimplementedStringFromJni());
        setContentView(tv);
    }

    public native String stringFromJni();

    public native String unimplementedStringFromJni();

    static {
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avformat-57");
        System.loadLibrary("swresample-2");
        System.loadLibrary("swscale-4");
        System.loadLibrary("avutil-55");
        System.loadLibrary("hello-jni");
    }
}
