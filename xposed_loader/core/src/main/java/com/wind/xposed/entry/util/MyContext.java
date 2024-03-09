package com.wind.xposed.entry.util;

import android.content.Context;
import android.content.ContextWrapper;

public class MyContext extends ContextWrapper {
    public MyContext(Context base) {
        super(base);
    }

    @Override
    public Context getApplicationContext() {
        return getBaseContext();
    }
}
