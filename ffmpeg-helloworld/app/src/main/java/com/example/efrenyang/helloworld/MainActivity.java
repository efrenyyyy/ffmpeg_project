package com.example.efrenyang.helloworld;

import android.support.v7.app.AppCompatActivity;
import android.app.Activity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final TextView libinfoText = (TextView) findViewById(R.id.text_libinfo);
        libinfoText.setMovementMethod(ScrollingMovementMethod.getInstance());

        libinfoText.setText(configurationinfo());

        Button configurationButton = (Button) this.findViewById(R.id.button_configuration);
        Button urlprotocolButton = (Button) this.findViewById(R.id.button_urlprotocol);
        Button avformatButton = (Button) this.findViewById(R.id.button_avformat);
        Button avcodecButton = (Button) this.findViewById(R.id.button_avcodec);
        Button avfilterbutton = (Button) this.findViewById(R.id.button_avfilter);

        urlprotocolButton.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                libinfoText.setText(urlprotocolinfo());
            }
        });

        avformatButton.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                libinfoText.setText(avformatinfo());
            }
        });

        avcodecButton.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                libinfoText.setText(avcodecinfo());
            }
        });

        avfilterbutton.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                libinfoText.setText(avfilterinfo());
            }
        });

        configurationButton.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                libinfoText.setText(configurationinfo());
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu){
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    public native String urlprotocolinfo();
    public native String avformatinfo();
    public native String avcodecinfo();
    public native String avfilterinfo();
    public native String configurationinfo();

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
