#ifndef KS_WEB_HTML_H
#define KS_WEB_HTML_H

#include <Arduino.h>

const char indexHTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>KSmart Relay Admin</title>
    <style>
        * {
            box-sizing: border-box;
        }
        :root {
            --primary: #4361ee;
            --success: #2ecc71;
            --danger: #e74c3c;
            --bg: #f8f9fa;
            --card-bg: #ffffff;
            --text: #2b2d42;
            --border: #e0e0e0;
        }
        body { 
            font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; 
            background-color: var(--bg); 
            color: var(--text);
            margin: 0; 
            padding: 20px; 
            line-height: 1.6;
        }
        .container { 
            max-width: 800px;
            margin: auto; 
            background: var(--card-bg); 
            padding: 30px; 
            border-radius: 12px; 
            box-shadow: 0 4px 6px rgba(0,0,0,0.05), 0 1px 3px rgba(0,0,0,0.1); 
        }
        .header-actions {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 30px;
            position: sticky;
            background-color: var(--card-bg);
            top: 0;
            padding: 20px;
            z-index: 100;
        }
        h1 { 
            color: var(--primary); 
            margin: 0;
            font-weight: 600;
        }
        .btn-edit {
            padding: 10px 20px;
            background-color: var(--primary);
            color: white;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-weight: 500;
            transition: background-color 0.2s;
        }
        .btn-edit.active {
            background-color: var(--danger);
        }
        .device-group { 
            margin-bottom: 25px; 
            padding: 20px; 
            border-radius: 8px; 
            background-color: #fafbfc; 
            transition: transform 0.2s ease;
        }
        .device-group:hover {
            box-shadow: 0 2px 8px rgba(0,0,0,0.05);
        }
        .global-actions {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
            padding: 0 20px;
        }
        .btn-all-on {
            flex: 1;
            padding: 12px;
            background-color: var(--primary);
            color: white;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 600;
            transition: all 0.2s;
        }
        .btn-all-off {
            flex: 1;
            padding: 12px;
            background-color: var(--primary);
            color: white;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-weight: 600;
            transition: all 0.2s;
        }
              .device-title { 
            font-size: 1em; 
            font-weight: 600; 
            color: var(--primary); 
        }
        .relay-list {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
            gap: 15px;
        }
        .relay-item { 
            display: flex; 
            align-items: center; 
            background: white;
            padding: 10px;
            border-radius: 6px;
            gap: 10px;
        }
        .relay-id { 
            width: 45px; 
            font-weight: bold; 
            color: #6c757d;
            background: #e9ecef;
            padding: 5px;
            text-align: center;
            border-radius: 4px;
            flex-shrink: 0;
        }
        .relay-name { 
            flex: 1; 
            min-width: 0;
            width: 100%;
            padding: 8px 12px; 
            border: 1px solid #ced4da; 
            border-radius: 4px; 
            font-size: 1em;
            transition: border-color 0.15s ease-in-out, box-shadow 0.15s ease-in-out;
            background-color: #fff;
        }
        .footer-actions{
            position: sticky;
            bottom: 0;
            z-index: 100;
            padding: 10px;
            background: var(--card-bg);
        }
        .relay-name:disabled {
            background-color: #f1f3f5;
            color: #495057;
            cursor: not-allowed;
            border-color: #dee2e6;
        }
        .relay-name:focus {
            outline: 0;
            border-color: #80bdff;
            box-shadow: 0 0 0 0.2rem rgba(67, 97, 238, 0.25);
        }
        .btn-save { 
            display: block; 
            width: 100%; 
            padding: 15px; 
            background-color: var(--primary); 
            color: white; 
            border: none; 
            border-radius: 8px; 
            font-size: 1.1em; 
            font-weight: 600;
            cursor: pointer; 
            margin-top: 30px; 
            transition: background-color 0.2s, transform 0.1s;
        }
        .settings-card {
            background-color: #fafbfc; 
            padding: 20px;
            border-radius: 12px;
            margin-bottom: 20px;
        }
        .settings-group {
            display: flex;
            justify-content: space-between;
            align-items: center;
            gap: 15px;
            flex-wrap: wrap;
        }
        .settings-group label {
            font-weight: 500;
            flex: 1;
            min-width: 200px;
        }
        .settings-input {
            width: 120px;
            padding: 10px 15px;
            border: 1px solid var(--border);
            border-radius: 8px;
            background-color: white;
            font-size: 0.95em;
            font-weight: 600;
            color: var(--text);
            cursor: pointer;
            appearance: none;
            -webkit-appearance: none;
            background-image: url("data:image/svg+xml;charset=UTF-8,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%234361ee' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3e%3cpolyline points='6 9 12 15 18 9'%3e%3c/polyline%3e%3c/svg%3e");
            background-repeat: no-repeat;
            background-position: right 10px center;
            background-size: 16px;
            transition: all 0.2s ease;
        }
        .settings-input:focus {
            outline: none;
            border-color: var(--primary);
            box-shadow: 0 0 0 3px rgba(67, 97, 238, 0.15);
        }
        .settings-input:disabled {
            background-color: #f1f3f5;
            border-color: #dee2e6;
            color: #6c757d;
            cursor: not-allowed;
            background-image: none;
            padding-right: 15px;
            width: 100px;
            text-align: center;
        }
        
        /* Custom Select */
        .custom-select-wrapper {
            position: relative;
            user-select: none;
            flex: 2;
            min-width: 120px;
            transition: background-color 0.3s, border-color 0.3s, color 0.3s, opacity 0.3s;
        }
        .custom-select-trigger {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 10px 15px;
            font-size: 0.95em;
            font-weight: 600;
            color: var(--text);
            background: #fff;
            border: 1px solid var(--border);
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.2s;
        }
        .custom-select-wrapper.open .custom-select-trigger {
            border-bottom-left-radius: 0;
            border-bottom-right-radius: 0;
        }
        .custom-select-trigger:after {
            content: "";
            width: 12px;
            height: 12px;
            background-image: url("data:image/svg+xml;charset=UTF-8,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%234361ee' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3e%3cpolyline points='6 9 12 15 18 9'%3e%3c/polyline%3e%3c/svg%3e");
            background-size: contain;
            background-repeat: no-repeat;
            transition: transform 0.3s;
        }
        .custom-select-wrapper.open .custom-select-trigger:after {
            transform: rotate(180deg);
        }
        .custom-options {
            position: absolute;
            display: block;
            top: 100%;
            left: 0;
            right: 0;
            border: 1px solid var(--border);
            border-top: 0;
            background: #fff;
            transition: all 0.3s;
            opacity: 0;
            visibility: hidden;
            pointer-events: none;
            transform: translateY(-10px);
            border-radius: 0 0 8px 8px;
            box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1);
            z-index: 100;
            max-height: 250px;
            overflow-y: auto;
            -ms-overflow-style: none;  /* IE and Edge */
            scrollbar-width: none;  /* Firefox */
        }
        .custom-options::-webkit-scrollbar {
            display: none; /* Chrome, Safari and Opera */
        }
        .custom-select-wrapper.open .custom-options {
            opacity: 1;
            visibility: visible;
            pointer-events: all;
            transform: translateY(0);
        }
        .custom-option {
            position: relative;
            display: block;
            padding: 12px 15px;
            font-size: 0.95em;
            font-weight: 500;
            color: var(--text);
            cursor: pointer;
            transition: all 0.2s;
            border-bottom: 1px solid #f1f3f5;
        }
        .custom-option:last-child {
            border-bottom: none;
        }
        .custom-option:hover {
            background: #f8f9fa;
            color: var(--primary);
        }
        .custom-option.selected {
            color: #fff;
            background: var(--primary);
        }
        .custom-select-wrapper.disabled {
            pointer-events: none;
            max-width: 100px;
            min-width: unset;
        }
        .custom-select-wrapper.disabled .custom-select-trigger {
            background-color: #f1f3f5;
            border-color: #dee2e6;
            color: #6c757d;
        }
        .custom-select-wrapper.disabled .custom-select-trigger:after {
            display: none;
        }
        .btn-save:hover { 
            background-color: #3f37c9; 
        }
        .btn-save:active {
            transform: scale(0.98);
        }
        .notification { 
            display: none; 
            padding: 15px; 
            margin-bottom: 25px; 
            border-radius: 8px; 
            text-align: center; 
            font-weight: 500;
            animation: fadeIn 0.3s;
        }
        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(-10px); }
            to { opacity: 1; transform: translateY(0); }
        }
        .success { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        
        .loader {
            border: 4px solid #f3f3f3;
            border-top: 4px solid var(--primary);
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 20px auto;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        
        /* Overlay mất kết nối */
        .disconnect-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100vw;
            height: 100vh;
            background-color: rgba(0, 0, 0, 0.75);
            backdrop-filter: blur(5px);
            z-index: 10000;
            display: none;
            justify-content: center;
            align-items: center;
            flex-direction: column;
            color: white;
            font-family: inherit;
        }
        .disconnect-card {
            background: rgba(30, 30, 30, 0.95);
            border: 1px solid rgba(255, 255, 255, 0.1);
            padding: 35px 50px;
            border-radius: 16px;
            text-align: center;
            box-shadow: 0 20px 40px rgba(0,0,0,0.5);
            max-width: 90%;
            width: 400px;
            animation: fadeIn 0.3s ease;
        }
        .disconnect-icon {
            font-size: 3.5em;
            margin-bottom: 15px;
            animation: pulse 1.5s infinite;
        }
        @keyframes pulse {
            0% { transform: scale(1); opacity: 0.6; }
            50% { transform: scale(1.08); opacity: 1; }
            100% { transform: scale(1); opacity: 0.6; }
        }
    </style>
</head>
<body>
    <div id="login-screen" class="login-screen">
        <div class="login-card">
            <div style="text-align: center; margin-bottom: 30px;">
                <h2 style="color: var(--primary); margin: 0;">KSmart Zone Admin</h2>
                <p style="color: #6c757d; margin-top: 10px;">Vui lòng đăng nhập để tiếp tục</p>
            </div>
            <div class="login-form-group">
                <label>Tài khoản</label>
                <input type="text" id="username" placeholder="Nhập tài khoản" autocomplete="username">
            </div>
            <div class="login-form-group">
                <label>Mật khẩu</label>
                <div class="password-wrapper">
                    <input type="password" id="password" placeholder="Nhập mật khẩu" autocomplete="current-password">
                    <span id="toggle-pwd" class="toggle-password" onclick="togglePasswordVisibility()">👁️</span>
                </div>
            </div>
            <div id="login-error" class="error-msg"></div>
            <button class="btn-save" onclick="handleLogin()" style="margin-top: 20px;">Đăng Nhập</button>
        </div>
    </div>

    <div class="container" id="main-container" style="display: none;">
        <div class="header-actions">
            <h1>Cấu Hình Thiết Bị KSmart</h1>
            <div style="display: flex; gap: 10px;">
                <button id="edit-mode-btn" class="btn-edit" onclick="toggleEditMode()" style="display:none;">Đổi tên công tắc</button>
                <button id="logout-btn" class="btn-edit" onclick="handleLogout()" style="background-color: var(--primary);">Đăng xuất</button>
            </div>
        </div>
        <div id="notification" class="notification"></div>
        
        <div id="system-settings" class="settings-card" style="display:none;">
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;">
                <h2 style="margin: 0; font-size: 1.2em; color: var(--primary);">Cấu Hình Hệ Thống</h2>
                <button id="edit-system-btn" class="btn-edit-id" onclick="toggleSystemEdit()">Chỉnh sửa</button>
            </div>
            <div class="settings-group">
                <label for="all-relay-delay">Độ trễ Bật/Tắt tất cả (giây):</label>
                <div class="custom-select-wrapper disabled" id="wrapper-delay">
                    <div class="custom-select-trigger" onclick="toggleCustomSelect('wrapper-delay')">1s</div>
                    <div class="custom-options">
                        <span class="custom-option" data-value="100" onclick="selectOption('wrapper-delay', '100', '0.1')">0.1s</span>
                        <span class="custom-option" data-value="300" onclick="selectOption('wrapper-delay', '300', '0.3')">0.3s</span>
                        <span class="custom-option" data-value="500" onclick="selectOption('wrapper-delay', '500', '0.5')">0.5s</span>
                        <span class="custom-option" data-value="1000" onclick="selectOption('wrapper-delay', '1000', '1')">1s</span>
                        <span class="custom-option" data-value="2000" onclick="selectOption('wrapper-delay', '2000', '2')">2s</span>
                        <span class="custom-option" data-value="5000" onclick="selectOption('wrapper-delay', '5000', '5')">5s</span>
                    </div>
                    <input type="hidden" id="all-relay-delay" value="1000">
                </div>
            </div>
            <div class="settings-group" style="margin-top: 15px; padding-top: 15px; border-top: 1px solid #eee;">
                <label for="screen-sleep-time">Thời gian tắt màn hình:</label>
                <div class="custom-select-wrapper disabled" id="wrapper-sleep">
                    <div class="custom-select-trigger" onclick="toggleCustomSelect('wrapper-sleep')">5 phút</div>
                    <div class="custom-options">
                        <span class="custom-option" data-value="30" onclick="selectOption('wrapper-sleep', '30', '30 giây')">30 giây</span>
                        <span class="custom-option" data-value="60" onclick="selectOption('wrapper-sleep', '60', '1 phút')">1 phút</span>
                        <span class="custom-option" data-value="120" onclick="selectOption('wrapper-sleep', '120', '2 phút')">2 phút</span>
                        <span class="custom-option" data-value="180" onclick="selectOption('wrapper-sleep', '180', '3 phút')">3 phút</span>
                        <span class="custom-option" data-value="240" onclick="selectOption('wrapper-sleep', '240', '4 phút')">4 phút</span>
                        <span class="custom-option" data-value="300" onclick="selectOption('wrapper-sleep', '300', '5 phút')">5 phút</span>
                        <span class="custom-option" data-value="600" onclick="selectOption('wrapper-sleep', '600', '10 phút')">10 phút</span>
                    </div>
                    <input type="hidden" id="screen-sleep-time" value="300">
                </div>
            </div>
            <div class="settings-group" style="margin-top: 15px; padding-top: 15px; border-top: 1px solid #eee;">
                <label for="used-channels-count" id="used-channels-label">Số cổng sử dụng (0 = Tất cả):</label>
                <div style="display: flex; align-items: center; gap: 8px;">
                    <input type="number" id="used-channels-count" class="settings-input" style="width: 100px;" min="0" value="0" disabled>
                    <span id="max-channels-label" style="font-weight: 500; color: #6c757d;">/ --</span>
                </div>
            </div>
        </div>

        <div id="wifi-settings" class="settings-card" style="display:none;">
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;">
                <h2 style="margin: 0; font-size: 1.2em; color: var(--primary);">Cấu Hình Wi-Fi (Dự Phòng)</h2>
            </div>
            <div class="settings-group">
                <label>Trạng thái:</label>
                <span id="wifi-status-text" style="font-weight: 600; color: #6c757d;">Đang kiểm tra...</span>
            </div>
            <div class="settings-group" style="margin-top: 15px; padding-top: 15px; border-top: 1px solid #eee;">
                <label>Tên Wi-Fi (SSID):</label>
                <input type="text" id="wifi-ssid" class="settings-input" style="width: 200px; text-align: left; background-image: none;" placeholder="Tên mạng">
            </div>
            <div class="settings-group" style="margin-top: 15px; padding-top: 15px; border-top: 1px solid #eee;">
                <label>Mật khẩu:</label>
                <input type="password" id="wifi-pass" class="settings-input" style="width: 200px; text-align: left; background-image: none;" placeholder="Mật khẩu">
            </div>
            <button class="btn-save" onclick="handleConnectWifi()" style="margin-top: 20px; padding: 10px;">Kết Nối Wi-Fi</button>
        </div>

        <div class="global-actions">
            <button class="btn-all-on" onclick="handleToggleAll(true)">Bật tất cả</button>
            <button class="btn-all-off" onclick="handleToggleAll(false)">Tắt tất cả</button>
        </div>

        <div id="relays-container">
            <div class="loader"></div>
            <div style="text-align:center; color:#6c757d;">Đang tải dữ liệu...</div>
        </div>
        <div class="footer-actions">
        <button id="save-btn" class="btn-save" onclick="saveRelays()" style="display:none;">Lưu Cấu Hình</button>
        </div>
    </div>

    <style>
        .login-screen {
            position: fixed;
            top: 0; left: 0; right: 0; bottom: 0;
            background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            z-index: 9999;
        }
        .login-card {
            background: white;
            padding: 40px;
            border-radius: 16px;
            box-shadow: 0 15px 35px rgba(0,0,0,0.1);
            width: 90%;
            max-width: 400px;
        }
        .login-form-group {
            margin-bottom: 20px;
        }
        .login-form-group label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #495057;
        }
        .login-form-group input {
            width: 100%;
            padding: 12px 15px;
            border: 1px solid var(--border);
            border-radius: 8px;
            font-size: 1em;
            box-sizing: border-box;
        }
        .password-wrapper {
            position: relative;
        }
        .toggle-password {
            position: absolute;
            right: 15px;
            top: 50%;
            transform: translateY(-50%);
            cursor: pointer;
            user-select: none;
            font-size: 1.2em;
            opacity: 0.6;
        }
        .toggle-password:hover {
            opacity: 1;
        }
        .error-msg {
            color: var(--danger);
            font-size: 0.9em;
            margin-top: -10px;
            margin-bottom: 10px;
            display: none;
            font-weight: 500;
        }
        .device-title-container {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 15px;
            gap: 10px;
        }
        .device-title-container > div {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .device-title {
            font-size: 1.25em;
            font-weight: 600;
            color: var(--primary);
            margin: 0;
            margin-right: 10px;
        }
        .device-id-input {
            width: 60px;
            border: 1px solid #ced4da;
            border-radius: 4px;
            font-size: 1.25em;
            font-weight: bold;
            color: var(--primary);
            text-align: center;
        }
        .device-id-input:disabled {
            background-color: transparent;
            border-color: transparent;
            margin: 0;
            cursor: default;
            text-align: left;
            width: 60px;
        }
        .btn-edit-id {
            padding: 5px 15px;
            background-color: #6c757d;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 0.9em;
            transition: all 0.2s;
        }
        .btn-edit-id:hover {
            background-color: var(--primary);
        }
        .btn-edit-id.active {
            background-color: var(--danger);
        }
        /* Remove arrows/spinners from number input */
        .device-id-input::-webkit-outer-spin-button,
        .device-id-input::-webkit-inner-spin-button {
            -webkit-appearance: none;
            margin: 0;
        }

        @media (max-width: 640px) {
            body { padding: 10px; }
            .container { padding: 15px; border-radius: 8px; }
            .header-actions { 
                flex-direction: column; 
                gap: 15px; 
                padding: 15px; 
                text-align: center;
                top: 0;
                margin-bottom: 20px;
            }
            .header-actions h1 { font-size: 1.4em; }
            .relay-list { grid-template-columns: 1fr; }
            .relay-item { padding: 8px; gap: 8px; }
            .relay-id { width: 38px; font-size: 0.9em; }
            .relay-name { padding: 8px 10px; font-size: 0.95em; }
            .settings-group { flex-direction: column; align-items: stretch; gap: 5px; margin-bottom: 0px; }
            .settings-input { width: 100%; text-align: center; }
            .btn-edit-id { width: auto; }
            .footer-actions { padding: 10px; }
            .btn-save { margin-top: 15px; padding: 15px; }
            .login-card { padding: 25px; }
            .device-group { padding: 15px; }
        }
        /* Switch CSS */
        .switch {
            position: relative;
            display: inline-block;
            width: 44px;
            height: 24px;
            flex-shrink: 0;
        }
        .switch input { 
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 24px;
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 18px;
            width: 18px;
            left: 3px;
            bottom: 3px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }
        input:checked + .slider {
            background-color: var(--primary);
        }
        input:checked + .slider:before {
            transform: translateX(20px);
        }
    </style>

    <script>
        function toggleCustomSelect(wrapperId) {
            const wrapper = document.getElementById(wrapperId);
            if (wrapper.classList.contains('disabled')) return;
            
            // Close other dropdowns
            document.querySelectorAll('.custom-select-wrapper').forEach(w => {
                if (w.id !== wrapperId) w.classList.remove('open');
            });
            
            wrapper.classList.toggle('open');
        }

        function selectOption(wrapperId, value, label) {
            const wrapper = document.getElementById(wrapperId);
            const trigger = wrapper.querySelector('.custom-select-trigger');
            const input = wrapper.querySelector('input[type="hidden"]');
            
            trigger.textContent = label;
            input.value = value;
            
            wrapper.querySelectorAll('.custom-option').forEach(opt => {
                opt.classList.remove('selected');
                if (opt.dataset.value === value) opt.classList.add('selected');
            });
            
            wrapper.classList.remove('open');
        }

        // Close dropdowns when clicking outside
        window.addEventListener('click', function(e) {
            if (!e.target.closest('.custom-select-wrapper')) {
                document.querySelectorAll('.custom-select-wrapper').forEach(w => {
                    w.classList.remove('open');
                });
            }
        });

        function togglePasswordVisibility() {
            const pwdInput = document.getElementById('password');
            const toggleIcon = document.getElementById('toggle-pwd');
            if (pwdInput.type === 'password') {
                pwdInput.type = 'text';
                toggleIcon.textContent = '🙈';
            } else {
                pwdInput.type = 'password';
                toggleIcon.textContent = '👁️';
            }
        }

        function handleLogin() {
            const user = document.getElementById('username').value.trim();
            const pass = document.getElementById('password').value.trim();
            const errorMsg = document.getElementById('login-error');

            if (user === 'admin' && pass === 'kis@2026') {
                document.getElementById('login-screen').style.display = 'none';
                document.getElementById('main-container').style.display = 'block';
                document.getElementById('system-settings').style.display = 'block';
                localStorage.setItem('isLoggedIn', 'true');
                localStorage.setItem('userRole', 'admin');
                showNotification("Đăng nhập thành công", true);
                renderRelays();
            } else if (user === 'ksmart' && pass === 'ksmart2026') {
                document.getElementById('login-screen').style.display = 'none';
                document.getElementById('main-container').style.display = 'block';
                document.getElementById('system-settings').style.display = 'none';
                localStorage.setItem('isLoggedIn', 'true');
                localStorage.setItem('userRole', 'ksmart');
                showNotification("Đăng nhập thành công", true);
                renderRelays();
            } else {
                errorMsg.textContent = 'Sai tài khoản hoặc mật khẩu!';
                errorMsg.style.display = 'block';
            }
        }

        function handleLogout() {
            localStorage.removeItem('isLoggedIn');
            localStorage.removeItem('userRole');
            location.reload();
        }

        // Allow Enter key to login
        document.addEventListener('keydown', function(e) {
            if (e.key === 'Enter') {
                const loginScreen = document.getElementById('login-screen');
                if (loginScreen && loginScreen.style.display !== 'none') {
                    handleLogin();
                }
            }
        });

        let currentData = null;
        let isEditMode = false;
        let idEditDevices = new Set();
        let isSystemEditing = false;

        document.addEventListener("DOMContentLoaded", () => {
            // Kiểm tra trạng thái đăng nhập từ localStorage
            if (localStorage.getItem('isLoggedIn') === 'true') {
                document.getElementById('login-screen').style.display = 'none';
                document.getElementById('main-container').style.display = 'block';
                const role = localStorage.getItem('userRole');
                if (role === 'admin') {
                    document.getElementById('system-settings').style.display = 'block';
                    const wifiSettings = document.getElementById('wifi-settings');
                    if (wifiSettings) {
                        wifiSettings.style.display = 'block';
                        fetchWifiStatus();
                    }
                } else {
                    document.getElementById('system-settings').style.display = 'none';
                    const wifiSettings = document.getElementById('wifi-settings');
                    if (wifiSettings) wifiSettings.style.display = 'none';
                }
            }

            fetch('/api/relays')
                .then(response => {
                    if (!response.ok) throw new Error('Không thể tải dữ liệu từ server');
                    return response.json();
                })
                .then(data => {
                    currentData = data;
                    
                    let totalPhysical = 0;
                    if (currentData.devices) {
                        for (let devId in currentData.devices) {
                            if (devId === "0" && currentData['hide-device-id-0'] == 1) {
                                continue;
                            }
                            if (currentData.devices[devId].relays) {
                                totalPhysical += Object.keys(currentData.devices[devId].relays).length;
                            }
                        }
                    }
                    const maxLabel = document.getElementById('max-channels-label');
                    const usedInput = document.getElementById('used-channels-count');
                    if (maxLabel && usedInput) {
                        maxLabel.textContent = '/ ' + totalPhysical;
                        usedInput.max = totalPhysical;
                    }
                    
                    renderRelays();
                    fetchRelayStatus();
                    document.getElementById('edit-mode-btn').style.display = 'block';
                })
                .catch(error => {
                    document.getElementById('relays-container').innerHTML = '';
                    showNotification('Lỗi khi tải dữ liệu: ' + error.message, false);
                });
        });

        function toggleEditMode() {
            isEditMode = !isEditMode;
            const btn = document.getElementById('edit-mode-btn');
            const saveBtn = document.getElementById('save-btn');
            const nameInputs = document.querySelectorAll('.relay-name');

            if (isEditMode) {
                btn.textContent = 'Hủy chỉnh sửa';
                btn.classList.add('active');
                saveBtn.style.display = 'block';
                nameInputs.forEach(input => input.disabled = false);
            } else {
                btn.textContent = 'Đổi tên công tắc';
                btn.classList.remove('active');
                if (idEditDevices.size === 0) saveBtn.style.display = 'none';
                nameInputs.forEach(input => input.disabled = true);
                if (idEditDevices.size === 0) renderRelays();
            }
            updateSaveButtonText();
        }

        function setCustomSelect(wrapperId, value) {
            const wrapper = document.getElementById(wrapperId);
            if (!wrapper) return;
            const input = wrapper.querySelector('input[type="hidden"]');
            const trigger = wrapper.querySelector('.custom-select-trigger');
            const options = wrapper.querySelectorAll('.custom-option');
            
            input.value = value;
            let found = false;
            options.forEach(opt => {
                opt.classList.remove('selected');
                if (opt.dataset.value == value) {
                    opt.classList.add('selected');
                    trigger.textContent = opt.textContent;
                    found = true;
                }
            });
            if (!found && options.length > 0) {
                // Fallback to first option if value not found
                const first = options[0];
                first.classList.add('selected');
                trigger.textContent = first.textContent;
                input.value = first.dataset.value;
            }
        }

        function toggleSystemEdit() {
            const saveBtn = document.getElementById('save-btn');
            const editBtn = document.getElementById('edit-system-btn');
            const delayWrapper = document.getElementById('wrapper-delay');
            const sleepWrapper = document.getElementById('wrapper-sleep');
            const usedChInput = document.getElementById('used-channels-count');

            isSystemEditing = !isSystemEditing;
            
            if (isSystemEditing) {
                delayWrapper.classList.remove('disabled');
                sleepWrapper.classList.remove('disabled');
                if(usedChInput) usedChInput.disabled = false;
                editBtn.textContent = 'Hủy chỉnh';
                editBtn.classList.add('active');
                saveBtn.style.display = 'block';
            } else {
                delayWrapper.classList.add('disabled');
                sleepWrapper.classList.add('disabled');
                if(usedChInput) { usedChInput.disabled = true; usedChInput.value = currentData.used_channels || 0; }
                setCustomSelect('wrapper-delay', currentData.all_relay_delay_ms || 1000);
                setCustomSelect('wrapper-sleep', currentData.screen_sleep_seconds || 300);
                editBtn.textContent = 'Chỉnh sửa';
                editBtn.classList.remove('active');
                if (idEditDevices.size === 0 && !isEditMode) saveBtn.style.display = 'none';
            }
            updateSaveButtonText();
        }

        function toggleIdEdit(deviceId) {
            const saveBtn = document.getElementById('save-btn');
            const idInput = document.querySelector('.device-id-input[data-original-id="' + deviceId + '"]');
            const editIdBtn = document.querySelector('.btn-edit-id[data-device-id="' + deviceId + '"]');

            if (idEditDevices.has(deviceId)) {
                idEditDevices.delete(deviceId);
                idInput.disabled = true;
                idInput.value = deviceId; // reset
                editIdBtn.textContent = 'Đổi ID';
                editIdBtn.classList.remove('active');
            } else {
                idEditDevices.add(deviceId);
                idInput.disabled = false;
                editIdBtn.textContent = 'Hủy đổi';
                editIdBtn.classList.add('active');
            }

            if (idEditDevices.size > 0 || isEditMode) {
                saveBtn.style.display = 'block';
            } else {
                saveBtn.style.display = 'none';
            }
            updateSaveButtonText();
        }

        function updateSaveButtonText() {
            const saveBtn = document.getElementById('save-btn');
            if (idEditDevices.size > 0) {
                saveBtn.textContent = 'Lưu và khởi động lại';
            } else {
                saveBtn.textContent = 'Lưu Cấu Hình';
            }
        }

        let currentRelayStatuses = {};

        // Calculate 1-based physical channel index
        function getPhysicalChannel(targetDeviceId, targetRelayId) {
            let channel = 1;
            const deviceIds = Object.keys(currentData.devices).sort((a, b) => parseInt(a) - parseInt(b));
            for (const deviceId of deviceIds) {
                const relays = currentData.devices[deviceId].relays || {};
                const relayIds = Object.keys(relays).sort((a, b) => parseInt(a) - parseInt(b));
                for (const relayId of relayIds) {
                    if (deviceId == targetDeviceId && relayId == targetRelayId) {
                        return channel;
                    }
                    channel++;
                }
            }
            return -1;
        }

        async function handleToggleRelay(deviceId, relayId, state) {
            const channel = getPhysicalChannel(deviceId, relayId);
            if (channel === -1) return;
            
            try {
                const response = await fetch('/api/relay/control', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ channel: channel, state: state })
                });
                if (!response.ok) {
                    showNotification('Không thể điều khiển relay', false);
                    setTimeout(fetchRelayStatus, 500); // Revert UI
                }
            } catch (e) {
                console.error(e);
                showNotification('Lỗi kết nối khi điều khiển relay', false);
                setTimeout(fetchRelayStatus, 500); // Revert UI
            }
        }

        async function handleToggleAll(state) {
            try {
                const response = await fetch('/api/relay/control', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ channel: 0, state: state })
                });
                if (!response.ok) {
                    showNotification('Không thể thực hiện lệnh bật/tắt tất cả', false);
                } else {
                    showNotification(state ? 'Đang bật tất cả relay...' : 'Đang tắt tất cả relay...', true);
                }
            } catch (e) {
                console.error(e);
                showNotification('Lỗi kết nối khi điều khiển', false);
            }
        }

        function showDisconnectOverlay() {
            const overlay = document.getElementById('disconnect-overlay');
            if (overlay && overlay.style.display !== 'flex') {
                overlay.style.display = 'flex';
            }
        }

        function hideDisconnectOverlay() {
            const overlay = document.getElementById('disconnect-overlay');
            if (overlay && overlay.style.display !== 'none') {
                overlay.style.display = 'none';
            }
        }

        let isFetchingStatus = false;
        async function fetchRelayStatus() {
            if (!currentData || isFetchingStatus) return; // Tránh chạy song song khi mạng lag
            isFetchingStatus = true;
            
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 800); // Đợi tối đa 800ms
            
            try {
                const res = await fetch('/api/relay/status', { signal: controller.signal });
                clearTimeout(timeoutId);
                if (res.ok) {
                    const statuses = await res.json();
                    currentRelayStatuses = statuses;
                    updateSwitches();
                    hideDisconnectOverlay();
                } else {
                    showDisconnectOverlay();
                }
            } catch (e) {
                clearTimeout(timeoutId);
                console.error("Lỗi lấy trạng thái relay hoặc timeout:", e);
                showDisconnectOverlay();
            } finally {
                isFetchingStatus = false;
            }
        }

        function updateSwitches() {
            const switches = document.querySelectorAll('.switch input[type="checkbox"]');
            switches.forEach(sw => {
                const deviceId = sw.dataset.deviceId;
                const relayId = sw.dataset.relayId;
                const channel = getPhysicalChannel(deviceId, relayId);
                if (channel !== -1 && currentRelayStatuses[channel] !== undefined) {
                    sw.checked = currentRelayStatuses[channel];
                }
            });
        }

        async function fetchWifiStatus() {
            try {
                const res = await fetch('/api/wifi/status');
                if (res.ok) {
                    const data = await res.json();
                    const statusText = document.getElementById('wifi-status-text');
                    if (data.connected) {
                        statusText.innerHTML = `<span style="color:var(--success)">Đã kết nối (${data.ssid})</span> - IP: ${data.ip}`;
                    } else {
                        statusText.innerHTML = `<span style="color:var(--danger)">Chưa kết nối</span>`;
                    }
                }
            } catch (e) {
                console.error("Lỗi lấy trạng thái wifi:", e);
            }
        }

        async function handleConnectWifi() {
            const ssid = document.getElementById('wifi-ssid').value.trim();
            const password = document.getElementById('wifi-pass').value.trim();
            if (!ssid) {
                showNotification("Vui lòng nhập tên Wi-Fi", false);
                return;
            }
            
            showNotification("Đang gửi yêu cầu kết nối...", true);
            try {
                const response = await fetch('/api/wifi/connect', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ ssid: ssid, password: password })
                });
                
                if (response.ok) {
                    showNotification("Đang kết nối, vui lòng chờ...", true);
                    // Lặp kiểm tra lại trạng thái sau 3s, 6s, 9s
                    setTimeout(fetchWifiStatus, 3000);
                    setTimeout(fetchWifiStatus, 6000);
                    setTimeout(fetchWifiStatus, 10000);
                } else {
                    const err = await response.text();
                    showNotification("Lỗi: " + err, false);
                }
            } catch (e) {
                showNotification("Lỗi mạng khi gọi kết nối", false);
            }
        }

        // Tăng tần suất lấy trạng thái từ ESP32 lên 500ms để hiển thị mượt mà hơn
        setInterval(fetchRelayStatus, 500);

        function renderRelays() {
            const container = document.getElementById('relays-container');
            container.innerHTML = '';

            if (!currentData) return;
            const delay = currentData.all_relay_delay_ms || 1000;
            const delayWrapper = document.getElementById('wrapper-delay');
            if (delayWrapper) {
                setCustomSelect('wrapper-delay', delay);
                if (isSystemEditing) delayWrapper.classList.remove('disabled');
                else delayWrapper.classList.add('disabled');
            }

            const sleepTime = currentData.screen_sleep_seconds || 300;
            const sleepWrapper = document.getElementById('wrapper-sleep');
            if (sleepWrapper) {
                setCustomSelect('wrapper-sleep', sleepTime);
                if (isSystemEditing) sleepWrapper.classList.remove('disabled');
                else sleepWrapper.classList.add('disabled');
            }
            const usedChInput = document.getElementById('used-channels-count');
            if (usedChInput) {
                usedChInput.value = currentData.used_channels || 0;
                if (isSystemEditing) usedChInput.disabled = false;
                else usedChInput.disabled = true;
            }
            const editBtn = document.getElementById('edit-system-btn');
            if (editBtn) {
                editBtn.textContent = isSystemEditing ? 'Hủy chỉnh' : 'Chỉnh sửa';
                if (isSystemEditing) editBtn.classList.add('active');
                else editBtn.classList.remove('active');
            }

            if (!currentData.devices || Object.keys(currentData.devices).length === 0) {
                container.innerHTML = '<div style="text-align:center; padding: 20px; color: #6c757d;">Không tìm thấy thiết bị nào trong file cấu hình.</div>';
                return;
            }

            const deviceIds = Object.keys(currentData.devices).sort((a, b) => parseInt(a) - parseInt(b));
            const role = localStorage.getItem('userRole');

            for (const deviceId of deviceIds) {
                if (deviceId === "0" && currentData['hide-device-id-0'] == 1) {
                    continue; // Skip device 0 if hide config is enabled
                }

                const deviceData = currentData.devices[deviceId];
                const group = document.createElement('div');
                group.className = 'device-group';
                
                const titleContainer = document.createElement('div');
                titleContainer.className = 'device-title-container';

                const titleLeft = document.createElement('div');

                const titleLabel = document.createElement('span');
                titleLabel.className = 'device-title';
                titleLabel.textContent = 'Mã thiết bị : ';

                const idInput = document.createElement('input');
                idInput.type = 'number';
                idInput.className = 'device-id-input';
                idInput.value = deviceId;
                idInput.disabled = !idEditDevices.has(deviceId);
                idInput.dataset.originalId = deviceId;
                
                titleLeft.appendChild(titleLabel);
                titleLeft.appendChild(idInput);
                titleContainer.appendChild(titleLeft);

                // Tắt tính năng Đổi ID từ xa qua Web trên ESP32 vì tính ổn định và chưa hỗ trợ Modbus FC06 Broadcast
                // if (role === 'admin' && deviceId !== "0") {
                //     const editIdBtn = document.createElement('button');
                //     editIdBtn.className = 'btn-edit-id';
                //     editIdBtn.textContent = idEditDevices.has(deviceId) ? 'Hủy đổi' : 'Đổi ID';
                //     if (idEditDevices.has(deviceId)) editIdBtn.classList.add('active');
                //     editIdBtn.dataset.deviceId = deviceId;
                //     editIdBtn.onclick = () => toggleIdEdit(deviceId);
                //     titleContainer.appendChild(editIdBtn);
                // }
                
                group.appendChild(titleContainer);

                const relayList = document.createElement('div');
                relayList.className = 'relay-list';

                const relaysData = deviceData.relays || {};
                const relayIds = Object.keys(relaysData).sort((a, b) => parseInt(a) - parseInt(b));
                
                for (const relayId of relayIds) {
                    const relayName = relaysData[relayId];
                    
                    const item = document.createElement('div');
                    item.className = 'relay-item';
                    
                    const idSpan = document.createElement('span');
                    idSpan.className = 'relay-id';
                    idSpan.textContent = 'R' + relayId;
                    
                    const input = document.createElement('input');
                    input.type = 'text';
                    input.className = 'relay-name';
                    input.value = relayName;
                    input.dataset.deviceId = deviceId;
                    input.dataset.relayId = relayId;
                    input.placeholder = 'Nhập tên relay...';
                    input.minLength = 1;
                    input.maxLength = 30;
                    input.disabled = !isEditMode;
                    
                    const switchLabel = document.createElement('label');
                    switchLabel.className = 'switch';
                    const switchInput = document.createElement('input');
                    switchInput.type = 'checkbox';
                    switchInput.dataset.deviceId = deviceId;
                    switchInput.dataset.relayId = relayId;
                    const channel = getPhysicalChannel(deviceId, relayId);
                    if (channel !== -1 && currentRelayStatuses[channel] !== undefined) {
                        switchInput.checked = currentRelayStatuses[channel];
                    }
                    switchInput.onchange = (e) => handleToggleRelay(deviceId, relayId, e.target.checked);
                    
                    const sliderSpan = document.createElement('span');
                    sliderSpan.className = 'slider round';
                    
                    switchLabel.appendChild(switchInput);
                    switchLabel.appendChild(sliderSpan);
                    
                    item.appendChild(idSpan);
                    item.appendChild(input);
                    item.appendChild(switchLabel);
                    relayList.appendChild(item);
                }
                
                group.appendChild(relayList);
                container.appendChild(group);
            }
        }

        async function saveRelays() {
            if (!currentData) return;
            
            const btn = document.getElementById('save-btn');
            const originalText = btn.textContent;
            btn.textContent = 'Đang lưu...';
            btn.disabled = true;
            
            try {
                // Validate Relay Names
                const allNameInputs = document.querySelectorAll('.relay-name');
                for (const input of allNameInputs) {
                    const val = input.value.trim();
                    if (val.length < 1 || val.length > 30) {
                        throw new Error('Tên relay phải từ 1 đến 30 ký tự (Lỗi tại R' + input.dataset.relayId + ')');
                    }
                }

                // 0. Cập nhật All Delay
                const delayInput = document.getElementById('all-relay-delay');
                if (delayInput) {
                    let delay = parseInt(delayInput.value);
                    if (isNaN(delay) || delay < 100) delay = 1000;
                    currentData.all_relay_delay_ms = delay;
                }

                // 0.1 Cập nhật Screen Sleep
                const sleepInput = document.getElementById('screen-sleep-time');
                if (sleepInput) {
                    let sleep = parseInt(sleepInput.value);
                    if (isNaN(sleep) || sleep < 30) sleep = 300;
                    currentData.screen_sleep_seconds = sleep;
                }

                // 0.2 Cập nhật Số cổng sử dụng
                const usedChInput = document.getElementById('used-channels-count');
                if (usedChInput) {
                    let usedCh = parseInt(usedChInput.value);
                    if (isNaN(usedCh) || usedCh < 0) usedCh = 0;
                    if (usedChInput.max && usedCh > parseInt(usedChInput.max)) {
                        usedCh = parseInt(usedChInput.max);
                        usedChInput.value = usedCh;
                    }
                    currentData.used_channels = usedCh;
                }

                // Thay vì đổi ID vật lý (chưa hỗ trợ trên ESP32 Web), ta chỉ cập nhật lại JSON với ID hiện tại
                const idInputs = document.querySelectorAll('.device-id-input');
                const idChanges = []; // Không dùng nữa, giữ lại mảng rỗng để tương thích code dưới
                
                // 2. Cập nhật dữ liệu JSON (chỉ cập nhật tên relay)
                const newDevices = {};
                const nameInputs = document.querySelectorAll('.relay-name');
                
                // Đầu tiên, cập nhật ID trong currentData
                for (const input of idInputs) {
                    const oldId = input.dataset.originalId;
                    const newId = input.value.trim();
                    if (newId === "") continue;
                    
                    const deviceData = JSON.parse(JSON.stringify(currentData.devices[oldId]));
                    newDevices[newId] = deviceData;
                }

                // Sau đó cập nhật tên relay cho các ID mới
                nameInputs.forEach(input => {
                    const oldDeviceId = input.dataset.deviceId;
                    // Tìm newId tương ứng với oldDeviceId
                    let newId = oldDeviceId;
                    for (const idInput of idInputs) {
                        if (idInput.dataset.originalId === oldDeviceId) {
                            newId = idInput.value.trim();
                            break;
                        }
                    }
                    
                    const relayId = input.dataset.relayId;
                    const newName = input.value.trim();
                    
                    if (newDevices[newId] && newDevices[newId].relays) {
                        newDevices[newId].relays[relayId] = newName;
                    }
                });

                // Khôi phục lại device 0 nếu nó đang bị ẩn để không bị xóa mất
                if (currentData['hide-device-id-0'] == 1 && currentData.devices && currentData.devices["0"]) {
                    newDevices["0"] = JSON.parse(JSON.stringify(currentData.devices["0"]));
                }

                currentData.devices = newDevices;

                // 3. Lưu JSON lên server
                const saveResponse = await fetch('/api/relays', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(currentData)
                });

                if (idChanges.length > 0) {
                    showNotification('Đã đổi ID thành công. Đang lưu cấu hình và chuẩn bị reboot...', true);
                } else {
                    showNotification('Đã lưu cấu hình thành công!', true);
                }
                
                isEditMode = false;
                idEditDevices.clear();
                isSystemEditing = false;
                renderRelays();
                const editBtn = document.getElementById('edit-mode-btn');
                editBtn.textContent = 'Chỉnh sửa';
                editBtn.classList.remove('active');
                btn.style.display = 'none';

                // 4. Nếu có đổi ID, thực hiện reboot sau một khoảng thời gian ngắn để server kịp phản hồi
                if (idChanges.length > 0) {
                    setTimeout(() => {
                        fetch('/api/reboot', { method: 'POST' })
                        .then(() => {
                            showNotification('Đang khởi động lại thiết bị... Vui lòng đợi.', true);
                        })
                        .catch(err => console.error('Reboot failed:', err));
                    }, 1000);
                }

            } catch (error) {
                showNotification(error.message, false);
            } finally {
                btn.textContent = originalText;
                btn.disabled = false;
            }
        }

        function showNotification(message, isSuccess) {
            const notif = document.getElementById('notification');
            notif.textContent = message;
            notif.className = 'notification ' + (isSuccess ? 'success' : 'error');
            notif.style.display = 'block';
            notif.style.opacity = '1';
            
            setTimeout(() => {
                (function fade() {
                    if ((notif.style.opacity -= .1) < 0) {
                        notif.style.display = "none";
                    } else {
                        requestAnimationFrame(fade);
                    }
                })();
            }, 3000);
        }
    </script>
    <div id="disconnect-overlay" class="disconnect-overlay">
        <div class="disconnect-card">
            <div class="disconnect-icon">⚠️</div>
            <h2 style="margin: 0; font-size: 1.4em; font-weight: 600; color: #ff4d4d;">Mất Kết Nối Hệ Thống</h2>
            <p style="margin: 10px 0 0 0; color: #cccccc; font-size: 0.95em;">Vui lòng kiểm tra lại cáp mạng LAN hoặc nguồn của thiết bị.</p>
        </div>
    </div>
</body>
</html>
)=====";

#endif // KS_WEB_HTML_H
