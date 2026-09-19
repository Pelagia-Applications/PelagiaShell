#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

import requests


def load_env_file():
    env_file = Path(__file__).resolve().parent / '.env'
    if not env_file.exists():
        return

    for line in env_file.read_text(encoding='utf-8').splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith('#') or '=' not in stripped:
            continue

        key, value = stripped.split('=', 1)
        os.environ.setdefault(key.strip(), value.strip().strip('"').strip("'"))


load_env_file()


def get_ipinfo():
    token = os.environ.get('IPINFO_TOKEN')
    url = 'https://ipinfo.io/json' if not token else f'https://ipinfo.io/json?token={token}'

    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        payload = response.json()
        return payload.get('city'), payload.get('region'), payload.get('country'), payload.get('loc')
    except requests.RequestException:
        return None, None, None, None


def get_city_from_args(args):
    return ' '.join(args) if args else None


def get_weather_data(city_query: str, api_key: str):
    params = {
        'q': city_query,
        'appid': api_key,
        'units': 'metric'
    }
    response = requests.get('https://api.openweathermap.org/data/2.5/weather', params=params, timeout=10)
    response.raise_for_status()
    data = response.json()
    if 'main' not in data:
        raise ValueError(data.get('message', 'weather data unavailable'))
    return data


def format_weather_summary(data):
    city_name = data.get('name', 'Unknown')
    country_name = data.get('sys', {}).get('country', '')
    desc = data.get('weather', [{}])[0].get('description', 'unknown').title()
    temp = data.get('main', {}).get('temp', 0)
    feels_like = data.get('main', {}).get('feels_like', temp)
    humidity = data.get('main', {}).get('humidity', 0)
    wind_speed = data.get('wind', {}).get('speed', 0)
    clouds = data.get('clouds', {}).get('all', 0)

    label = f'{city_name}, {country_name}' if country_name else city_name
    return (
        f'{label} -> {desc} | {temp}°C (feels like {feels_like}°C) '
        f'| humidity {humidity}% | wind {wind_speed} m/s | clouds {clouds}%'
    )


def main():
    api_key = os.environ.get('OPENWEATHER_API_KEY')
    if not api_key:
        print('weather: OPENWEATHER_API_KEY is not set.')
        print('Add it to scripts/.env as: OPENWEATHER_API_KEY="your_key"')
        print('Optional: IPINFO_TOKEN="your_ipinfo_token"')
        return 1

    args = sys.argv[1:]
    query = get_city_from_args(args)

    if not query:
        city, region, country, loc = get_ipinfo()
        if city and region and country:
            query = f'{city},{region},{country}'
        elif loc:
            query = loc
        else:
            print('weather: no location provided and IP detection failed.')
            print('Usage: weather <city> or weather <city,region,country>')
            return 1

    try:
        data = get_weather_data(query, api_key)
        print(format_weather_summary(data))
        return 0
    except requests.RequestException as exc:
        print(f'weather: unable to fetch forecast: {exc}')
        return 1
    except (KeyError, ValueError, json.JSONDecodeError) as exc:
        print(f'weather: unexpected response: {exc}')
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
