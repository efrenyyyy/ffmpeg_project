package com.example.efrenyang.ffmpeg_decoder;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button startButton = (Button)this.findViewById(R.id.button_start);
        final EditText url_input = (EditText)this.findViewById(R.id.input_url);
        final EditText url_output = (EditText)this.findViewById(R.id.output_url);

        startButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String folderurl = Environment.getExternalStorageDirectory().getPath();
                String urltext_input = url_input.getText().toString();
                urltext_input = folderurl + "/" + urltext_input;

                String urltext_output = url_output.getText().toString();
                urltext_output = folderurl + "/" + urltext_output;

                Log.i("input url", urltext_input);
                Log.i("output url", urltext_output);

                decode(urltext_input, urltext_output);
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String decode(String inputurl, String outputurl);

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("decoder-lib");
        System.loadLibrary("ffmpeg");
    }
}
