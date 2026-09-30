<?php

declare(strict_types=1);

const STATE_DIR = __DIR__ . '/../devices';

function str_starts_with_compat(string $haystack, string $needle): bool
{
    if ($needle == '') {
        return true;
    }
    return substr($haystack, 0, strlen($needle)) === $needle;
}

function str_ends_with_compat(string $haystack, string $needle): bool
{
    if ($needle == '') {
        return true;
    }
    $needleLen = strlen($needle);
    if ($needleLen > strlen($haystack)) {
        return false;
    }
    return substr($haystack, -$needleLen) === $needle;
}

function set_cors_headers(): void
{
    header('Access-Control-Allow-Origin: *');
    header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
    header('Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With');
    header('Access-Control-Max-Age: 86400');
}

function send_json(int $statusCode, array $payload): void
{
    http_response_code($statusCode);
    set_cors_headers();
    header('Content-Type: application/json; charset=utf-8');
    header('Cache-Control: no-store');
    echo json_encode($payload, JSON_UNESCAPED_SLASHES);
    exit;
}

function ensure_state_dir(): void
{
    if (!is_dir(STATE_DIR) && !mkdir(STATE_DIR, 0775, true) && !is_dir(STATE_DIR)) {
        send_json(500, ['error' => 'state_dir_create_failed']);
    }

    if (!is_writable(STATE_DIR)) {
        send_json(500, ['error' => 'state_dir_not_writable']);
    }
}

function normalize_base_path(string $path): string
{
    $scriptName = str_replace('\\', '/', $_SERVER['SCRIPT_NAME'] ?? '');
    $baseDir = str_replace('\\', '/', dirname($scriptName));
    if ($baseDir === '.' || $baseDir === '/') {
        $baseDir = '';
    }

    if ($baseDir !== '' && str_starts_with_compat($path, $baseDir . '/')) {
        $path = substr($path, strlen($baseDir));
    }

    if (str_starts_with_compat($path, '/index.php/')) {
        $path = substr($path, strlen('/index.php'));
    } elseif ($path === '/index.php') {
        $path = '/';
    }

    return $path;
}

function parse_route(): array
{
    $rawPath = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?: '/';
    $path = trim(normalize_base_path($rawPath), '/');
    if ($path === '') {
        send_json(404, [
            'error' => 'invalid_route',
            'hint' => 'use /api/devices or /api/<deviceId> or /api/<deviceId>/set',
        ]);
    }

    $parts = array_values(array_filter(explode('/', $path), static function (string $p): bool {
        return $p !== '';
    }));
    $scriptDir = trim(str_replace('\\', '/', dirname($_SERVER['SCRIPT_NAME'] ?? '')), '/');
    $scriptUnderApiDir = $scriptDir === 'api' || str_ends_with_compat($scriptDir, '/api');

    if (count($parts) >= 2 && $parts[0] === 'api') {
        $parts = array_slice($parts, 1);
    } elseif (!$scriptUnderApiDir) {
        send_json(404, [
            'error' => 'invalid_route',
            'hint' => 'use /api/devices or /api/<deviceId> or /api/<deviceId>/set',
        ]);
    }

    if (count($parts) < 1) {
        send_json(404, [
            'error' => 'invalid_route',
            'hint' => 'use /api/devices or /api/<deviceId> or /api/<deviceId>/set',
        ]);
    }

    if (count($parts) === 1 && strtolower($parts[0]) === 'devices') {
        return ['devices', null, false];
    }

    $rawDeviceId = $parts[0];
    if (!preg_match('/^[A-Za-z0-9][A-Za-z0-9-]{0,63}$/', $rawDeviceId)) {
        send_json(400, [
            'error' => 'invalid_device_id',
            'hint' => 'deviceId must match [A-Za-z0-9-], up to 64 chars',
        ]);
    }

    $deviceId = strtolower($rawDeviceId);

    $isSet = count($parts) === 2 && strtolower($parts[1]) === 'set';
    if (!(count($parts) === 1 || $isSet)) {
        send_json(404, [
            'error' => 'invalid_route',
            'hint' => 'use /api/devices or /api/<deviceId> or /api/<deviceId>/set',
        ]);
    }

    return ['device', $deviceId, $isSet];
}

function state_file(string $deviceId): string
{
    return STATE_DIR . '/' . $deviceId . '.json';
}

function find_existing_state_file(string $deviceId): ?string
{
    $candidate = state_file($deviceId);
    if (is_file($candidate)) {
        return $candidate;
    }

    $files = glob(STATE_DIR . '/*.json');
    if ($files === false) {
        return null;
    }

    foreach ($files as $file) {
        $name = pathinfo($file, PATHINFO_FILENAME);
        if (strtolower($name) === $deviceId) {
            return $file;
        }
    }

    return null;
}

function default_state(string $deviceId): array
{
    return [
        'deviceId' => $deviceId,
        'led' => false,
        'buzzer' => false,
        'button' => false,
        'motion' => false,
        'tempC' => null,
        'humidity' => null,
        'updatedAt' => gmdate('c'),
    ];
}

function read_state(string $deviceId): array
{
    $file = find_existing_state_file($deviceId);
    if ($file === null || !is_file($file)) {
        $state = default_state($deviceId);
        write_state($deviceId, $state);
        return $state;
    }

    $raw = file_get_contents($file);
    if ($raw === false || trim($raw) === '') {
        return default_state($deviceId);
    }

    $data = json_decode($raw, true);
    if (!is_array($data)) {
        return default_state($deviceId);
    }

    return array_merge(default_state($deviceId), $data, ['deviceId' => $deviceId]);
}

function write_state(string $deviceId, array $state): void
{
    $file = state_file($deviceId);
    $tmp = $file . '.tmp';

    $json = json_encode($state, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
    if ($json === false) {
        send_json(500, ['error' => 'state_encode_failed']);
    }

    if (file_put_contents($tmp, $json . PHP_EOL, LOCK_EX) === false) {
        send_json(500, ['error' => 'state_write_failed']);
    }

    if (!rename($tmp, $file)) {
        @unlink($tmp);
        send_json(500, ['error' => 'state_rename_failed']);
    }
}

function parse_bool_value($value): ?bool
{
    if (is_bool($value)) {
        return $value;
    }

    if (is_int($value) || is_float($value)) {
        if ($value == 1) {
            return true;
        }
        if ($value == 0) {
            return false;
        }
        return null;
    }

    if (is_string($value)) {
        $normalized = strtolower(trim($value));
        if (in_array($normalized, ['1', 'true', 'on', 'high'], true)) {
            return true;
        }
        if (in_array($normalized, ['0', 'false', 'off', 'low'], true)) {
            return false;
        }
    }

    return null;
}

function collect_set_inputs(): array
{
    $inputs = [];

    $jsonBody = [];
    $rawBody = file_get_contents('php://input');
    if (is_string($rawBody) && trim($rawBody) !== '') {
        $decoded = json_decode($rawBody, true);
        if (is_array($decoded)) {
            $jsonBody = $decoded;
        }
    }

    $sources = [$_GET, $_POST, $jsonBody];
    foreach ($sources as $source) {
        foreach (['led', 'buzz', 'buzzer', 'button', 'motion', 'tempC', 'humidity'] as $key) {
            if (array_key_exists($key, $source)) {
                $inputs[$key] = $source[$key];
            }
        }
    }

    return $inputs;
}

function parse_temp_value($value): array
{
    if ($value === null) {
        return [true, null];
    }

    if (is_string($value)) {
        $trimmed = trim($value);
        if ($trimmed === '' || strtolower($trimmed) === 'null') {
            return [true, null];
        }
        if (!is_numeric($trimmed)) {
            return [false, null];
        }
        $numeric = (float)$trimmed;
        return [is_finite($numeric), $numeric];
    }

    if (is_int($value) || is_float($value)) {
        $numeric = (float)$value;
        return [is_finite($numeric), $numeric];
    }

    return [false, null];
}

function list_all_states(): array
{
    $files = glob(STATE_DIR . '/*.json');
    if ($files === false) {
        return [];
    }

    sort($files, SORT_NATURAL | SORT_FLAG_CASE);
    $states = [];
    foreach ($files as $file) {
        $deviceId = strtolower((string)pathinfo($file, PATHINFO_FILENAME));
        $states[] = read_state($deviceId);
    }

    return $states;
}

ensure_state_dir();
set_cors_headers();

$method = strtoupper($_SERVER['REQUEST_METHOD'] ?? 'GET');
if ($method === 'OPTIONS') {
    http_response_code(204);
    exit;
}

[$routeType, $deviceId, $isSetRoute] = parse_route();

if ($routeType === 'devices') {
    if ($method !== 'GET') {
        send_json(405, ['error' => 'method_not_allowed', 'allowed' => ['GET']]);
    }

    $states = list_all_states();
    send_json(200, [
        'count' => count($states),
        'devices' => $states,
    ]);
}

if (!$isSetRoute) {
    if ($method !== 'GET') {
        send_json(405, ['error' => 'method_not_allowed', 'allowed' => ['GET']]);
    }

    $state = read_state($deviceId);
    send_json(200, $state);
}

if (!in_array($method, ['GET', 'POST'], true)) {
    send_json(405, ['error' => 'method_not_allowed', 'allowed' => ['GET', 'POST']]);
}

$state = read_state($deviceId);
$inputs = collect_set_inputs();

$hasLed = array_key_exists('led', $inputs);
$hasBuzz = array_key_exists('buzz', $inputs) || array_key_exists('buzzer', $inputs);
$hasButton = array_key_exists('button', $inputs);
$hasMotion = array_key_exists('motion', $inputs);
$hasTemp = array_key_exists('tempC', $inputs);
$hasHumidity = array_key_exists('humidity', $inputs);

if (!$hasLed && !$hasBuzz && !$hasButton && !$hasMotion && !$hasTemp && !$hasHumidity) {
    send_json(400, [
        'error' => 'missing_args',
        'hint' => 'use led and/or buzz and/or button and/or motion and/or tempC and/or humidity',
    ]);
}

if ($hasLed) {
    $parsed = parse_bool_value($inputs['led']);
    if ($parsed === null) {
        send_json(400, [
            'error' => 'invalid_led',
            'accepted' => ['1', '0', 'true', 'false', 'on', 'off'],
        ]);
    }
    $state['led'] = $parsed;
}

if ($hasBuzz) {
    $raw = array_key_exists('buzz', $inputs) ? $inputs['buzz'] : $inputs['buzzer'];
    $parsed = parse_bool_value($raw);
    if ($parsed === null) {
        send_json(400, [
            'error' => 'invalid_buzz',
            'accepted' => ['1', '0', 'true', 'false', 'on', 'off'],
        ]);
    }
    $state['buzzer'] = $parsed;
}

if ($hasButton) {
    $parsed = parse_bool_value($inputs['button']);
    if ($parsed === null) {
        send_json(400, [
            'error' => 'invalid_button',
            'accepted' => ['1', '0', 'true', 'false', 'on', 'off'],
        ]);
    }
    $state['button'] = $parsed;
}

if ($hasMotion) {
    $parsed = parse_bool_value($inputs['motion']);
    if ($parsed === null) {
        send_json(400, [
            'error' => 'invalid_motion',
            'accepted' => ['1', '0', 'true', 'false', 'on', 'off'],
        ]);
    }
    $state['motion'] = $parsed;
}

if ($hasTemp) {
    [$okTemp, $parsedTemp] = parse_temp_value($inputs['tempC']);
    if (!$okTemp) {
        send_json(400, [
            'error' => 'invalid_tempC',
            'accepted' => ['number', 'null'],
        ]);
    }
    $state['tempC'] = $parsedTemp;
}

if ($hasHumidity) {
    [$okHumidity, $parsedHumidity] = parse_temp_value($inputs['humidity']);
    if (!$okHumidity || ($parsedHumidity !== null && ($parsedHumidity < 0 || $parsedHumidity > 100))) {
        send_json(400, [
            'error' => 'invalid_humidity',
            'accepted' => ['number between 0 and 100', 'null'],
        ]);
    }
    $state['humidity'] = $parsedHumidity;
}

$state['deviceId'] = $deviceId;
$state['updatedAt'] = gmdate('c');
$state['lastMethod'] = $method;

write_state($deviceId, $state);

send_json(200, [
    'ok' => true,
    'deviceId' => $deviceId,
    'led' => $state['led'],
    'buzzer' => $state['buzzer'],
    'button' => $state['button'],
    'motion' => $state['motion'],
    'tempC' => $state['tempC'],
    'humidity' => $state['humidity'],
    'updatedAt' => $state['updatedAt'],
]);
