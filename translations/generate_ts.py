#!/usr/bin/env python3
"""Generate Qt .ts translation files from Python data dictionaries.

Usage:
    python3 generate_ts.py

This script reads translation data from JSON files matching the pattern
'translations_*.json' in the same directory and generates .ts files.

Each JSON file should have the structure:
{
    "language": "de",
    "translations": {
        "context_name": {
            "source_string": "translated_string",
            ...
        },
        ...
    }
}
"""

import json
import os
import glob
import xml.etree.ElementTree as ET
from xml.dom import minidom


def generate_ts_file(language, translations, output_dir):
    """Generate a single .ts XML file for a language.
    
    Args:
        language: Language code (e.g., "de")
        translations: Dict of {context: {source: translation}}
        output_dir: Directory to write the .ts file
    """
    # Build XML structure
    ts = ET.Element("TS")
    ts.set("version", "2.1")
    ts.set("language", language)
    
    for context_name, strings in sorted(translations.items()):
        context = ET.SubElement(ts, "context")
        name = ET.SubElement(context, "name")
        name.text = context_name
        
        for source, translation in sorted(strings.items()):
            message = ET.SubElement(context, "message")
            source_elem = ET.SubElement(message, "source")
            source_elem.text = source
            translation_elem = ET.SubElement(message, "translation")
            translation_elem.text = translation
    
    # Pretty print
    xml_str = minidom.parseString(ET.tostring(ts, encoding="unicode")).toprettyxml(indent="    ")
    
    # Remove extra XML declaration minidom adds (we want our own)
    lines = xml_str.split("\n")
    if lines[0].startswith("<?xml"):
        lines[0] = '<?xml version="1.0" encoding="utf-8"?>'
    
    output_path = os.path.join(output_dir, f"sailpush_{language}.ts")
    with open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    
    return output_path


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Find all translation JSON files
    json_pattern = os.path.join(script_dir, "translations_*.json")
    json_files = sorted(glob.glob(json_pattern))
    
    if not json_files:
        print("No translation JSON files found!")
        print(f"Looking in: {script_dir}")
        return 1
    
    print(f"Found {len(json_files)} translation file(s)")
    
    for json_file in json_files:
        with open(json_file, "r", encoding="utf-8") as f:
            data = json.load(f)
        
        language = data["language"]
        translations = data["translations"]
        
        output_path = generate_ts_file(language, translations, script_dir)
        num_contexts = len(translations)
        num_strings = sum(len(v) for v in translations.values())
        print(f"  Generated: {output_path} ({num_contexts} contexts, {num_strings} strings)")
    
    print("Done!")
    return 0


if __name__ == "__main__":
    exit(main())
