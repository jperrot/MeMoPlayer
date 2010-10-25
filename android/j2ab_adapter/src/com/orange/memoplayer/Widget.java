/*
 * Copyright (C) 2010 France Telecom
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.orange.memoplayer;

import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.Context;
import android.content.Intent;

import android.util.Log;


public class Widget extends AppWidgetProvider {
    
    public static int width, height;

    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {
        Log.d("Widget.UpdateService", "onUpdate()");
        if (appWidgetIds.length > 0) {
            width = appWidgetManager.getAppWidgetInfo(appWidgetIds[0]).minWidth;
            height = appWidgetManager.getAppWidgetInfo(appWidgetIds[0]).minHeight;
        }
        // Keep list of widgets to update
        //WidgetService.requestUpdate(appWidgetIds);
        // Perform the update in a service
        context.startService(new Intent(context, WidgetUpdate.class));
    }
}
