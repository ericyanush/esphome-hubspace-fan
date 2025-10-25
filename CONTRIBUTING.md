# Contributing to ESPHome HubSpace Component

Thank you for your interest in contributing to the ESPHome HubSpace Fan/Light component!

## How to Contribute

### Reporting Issues

If you encounter a bug or have a feature request:

1. Check if the issue already exists in the GitHub issue tracker
2. If not, create a new issue with:
   - Clear description of the problem or feature
   - Steps to reproduce (for bugs)
   - Expected vs actual behavior
   - Your ESPHome version and device type
   - Relevant configuration and logs

### Submitting Changes

1. **Fork the Repository**
   - Fork this repository to your GitHub account
   - Clone your fork locally

2. **Create a Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```
   or
   ```bash
   git checkout -b fix/your-bug-fix
   ```

3. **Make Your Changes**
   - Follow the existing code style
   - Add comments for complex logic
   - Update documentation if needed

4. **Test Your Changes**
   - Test with actual hardware if possible
   - Verify compilation succeeds
   - Check logs for errors

5. **Commit Your Changes**
   ```bash
   git add .
   git commit -m "Description of your changes"
   ```

6. **Push and Create Pull Request**
   ```bash
   git push origin feature/your-feature-name
   ```
   Then create a Pull Request on GitHub

## Code Style Guidelines

### Python Code
- Follow PEP 8 style guide
- Use 4 spaces for indentation
- Use descriptive variable names
- Add docstrings for functions

### C++ Code
- Follow ESPHome style guide
- Use 2 spaces for indentation
- Use descriptive variable names
- Add comments for complex logic
- Keep lines under 120 characters

### File Organization
```
components/hubspace/
  __init__.py          # Component config
  hubspace.h/cpp       # Base component
  fan.py/fan.h/cpp     # Fan platform
  light.py/light.h/cpp # Light platform
```

## Development Workflow

1. **Setup Development Environment**
   - Install ESPHome: `pip install esphome`
   - Clone the repository
   - Make your changes

2. **Test Locally**
   ```bash
   esphome compile example.yaml
   ```

3. **Validate Python Syntax**
   ```bash
   python3 -m py_compile components/hubspace/*.py
   ```

4. **Test on Hardware**
   - Flash to ESP device
   - Monitor logs
   - Test all features

## Areas for Contribution

### High Priority
- Implement actual HubSpace protocol communication
- Add device discovery/pairing
- Network communication layer

### Medium Priority
- Additional fan features (oscillation, direction)
- Color temperature support (if device supports)
- Improved error handling
- Better configuration options

### Nice to Have
- Unit tests
- Integration tests
- Additional documentation
- Example projects

## Questions?

If you have questions about contributing:
- Open a GitHub issue with the "question" label
- Check the DEVELOPMENT.md guide for technical details

## License

By contributing, you agree that your contributions will be licensed under the same license as this project (MIT License).

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Keep discussions on-topic

Thank you for contributing! 🎉
