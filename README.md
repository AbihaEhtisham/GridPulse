# GridPulse

Intelligent Decision Support Platform for Power Grid Resilience Analysis and Infrastructure Optimization.

## Project Structure

- `engine/` — C++17 simulation core
- `bindings/` — pybind11 Python bridge
- `backend/` — FastAPI web server
- `frontend/` — React + TypeScript + D3.js UI

## Quick Start

### Prerequisites
- C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.16+
- Python 3.10+
- Node.js 18+
- PostgreSQL 16 (or SQLite for development)

### Setup
```bash
# C++ Engine
cd engine && mkdir build && cd build
cmake .. && cmake --build .
./gridpulse_cli

# Backend
cd backend
python -m venv venv && source venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --reload

# Frontend
cd frontend
npm install && npm run dev