/*
 * Copyright (C) 2009 The J2AB Project
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

package j2ab.android.pim;

import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import android.database.Cursor;

public class PreloadingAndroidPIMItem extends AndroidPIMItem {

    private Map<String, Object> valueMappings;

    public PreloadingAndroidPIMItem(Cursor cursor,
            Map<Integer, String> columnMappings, Map<String, Class> typeMappings) {
        super (cursor, columnMappings);
        this.valueMappings = new HashMap<String, Object> (typeMappings.size ());
        this.preload (typeMappings);
    }

    protected void preload (Map<String, Class> typeMappings) {
        Iterator<String> keys = typeMappings.keySet ().iterator ();
        while (keys.hasNext ()) {
            String key = keys.next ();
            Class type = typeMappings.get (key);
            Object value;
            if (type == String.class) {
                value = super.getString (key);
            } else if (type == String[].class) {
                value = super.getStringArray (key);
            } else if (type == Date.class) {
                value = super.getDate (key);
            } else {
                value = null;
            }
            valueMappings.put (key, value);
        }
    }

    @Override
    public long getDate (int field, int index) {
        return ((Long) this.valueMappings.get (this.getColumnName (field)))
                .longValue ();
    }

    @Override
    public String getString (int field, int index) {
        return (String) this.valueMappings.get (this.getColumnName (field));
    }

    @Override
    public String[] getStringArray (int field, int index) {
        return (String[]) this.valueMappings.get (this.getColumnName (field));
    }

}
